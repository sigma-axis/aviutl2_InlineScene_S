/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <concepts>
#include <map>
#include <string>
using namespace std::string_literals;
using namespace std::string_view_literals;

#include <json.h>

#include "Settings.hpp"
#include "string_service.hpp"
using namespace InlineScene_S;
using namespace Settings;

#define ANON_NS_B namespace {
#define ANON_NS_E }

ANON_NS_B;
////////////////////////////////
// helper functions.
////////////////////////////////

// read from json.
template<class T>
bool parse_json_value(json_value_s* value, T& out) { static_assert(false); }

template<>
bool parse_json_value<int64_t>(json_value_s* value, int64_t& out)
{
	json_number_s* const number = json_value_as_number(value);
	if (number == nullptr) return false;

	std::string const number_string{ number->number, number->number_size };
	char* end = nullptr;
	auto const parsed = ::strtoll(number_string.c_str(), &end, 0);
	if (end == number_string.c_str()) return false;

	out = parsed;
	return true;
}

template<>
bool parse_json_value<double>(json_value_s* value, double& out)
{
	json_number_s* const number = json_value_as_number(value);
	if (number == nullptr) return false;

	std::string const number_string{ number->number, number->number_size };
	char* end = nullptr;
	double const parsed = ::strtod(number_string.c_str(), &end);
	if (end == number_string.c_str()) return false;

	out = parsed;
	return true;
}

template<>
bool parse_json_value<bool>(json_value_s* value, bool& out)
{
	if (json_value_is_true(value)) {
		out = true;
		return true;
	}
	if (json_value_is_false(value)) {
		out = false;
		return true;
	}
	return false;
}

template<>
bool parse_json_value<std::string>(json_value_s* value, std::string& out)
{
	json_string_s* const string = json_value_as_string(value);
	if (string == nullptr) return false;
	out.assign(string->string, string->string + string->string_size);
	return true;
}

template<>
bool parse_json_value<std::wstring>(json_value_s* value, std::wstring& out)
{
	std::string utf8;
	if (!parse_json_value(value, utf8)) return false;
	out = String_Service::to_wstring(utf8);
	return true;
}

// write to json.
template<class T>
std::string to_json_value_text(T const& value) { static_assert(false); }

template<> std::string to_json_value_text<int64_t>(int64_t const& value) { return std::to_string(value); }
template<> std::string to_json_value_text<double>(double const& value) { return std::to_string(value); }
template<> std::string to_json_value_text<bool>(bool const& value) { return value ? "true"s : "false"s; }

void escape_json_string_to(std::string& out, std::string_view const& input)
{
	out.reserve(out.size() + 2 * input.size());
	for (char ch : input) {
		switch (ch) {
		case '\\': out += "\\\\"sv; break;
		case '"': out += "\\\""sv; break;
		case '\n': out += "\\n"sv; break;
		case '\r': out += "\\r"sv; break;
		case '\t': out += "\\t"sv; break;
		default: out += ch; break;
		}
	}
}

template<>
std::string to_json_value_text<std::string>(std::string const& value)
{
	std::string result{}; result.reserve(2 + 2 * value.size());
	result += "\""sv;
	escape_json_string_to(result, value);
	result += "\""sv;
	return result;
}

template<>
std::string to_json_value_text<std::wstring>(std::wstring const& value)
{
	return to_json_value_text(String_Service::to_string(value));
}

// read narrow string from a file.
bool read_file(std::wstring const& file_path, std::string& content)
{
	FILE* file = nullptr;
	if (::_wfopen_s(&file, file_path.c_str(), L"rb") != 0 || file == nullptr) return false;

	bool success = false;
	if (::fseek(file, 0, SEEK_END) == 0) {
		if (auto const size = ::ftell(file); size >= 0) {
			::rewind(file);

			content.resize(static_cast<size_t>(size));
			success = size == 0 ||
				::fread(content.data(), 1, static_cast<size_t>(size), file) == static_cast<size_t>(size);
		}
	}

	::fclose(file);
	return success;
}

// write narrow string to a file.
bool write_file(std::wstring const& file_path, std::string const& content)
{
	FILE* file = nullptr;
	if (::_wfopen_s(&file, file_path.c_str(), L"wb") != 0 || file == nullptr) return false;

	bool const success =
		content.empty() ||
		::fwrite(content.data(), 1, content.size(), file) == content.size();

	::fclose(file);
	return success;
}

// field descriptor.
template<class ObjectT>
struct field_desc {
	void* ObjectT::* member_ptr;
	bool (field_desc::* read_ptr)(ObjectT& target, json_value_s* value) const;
	std::string(field_desc::* write_ptr)(ObjectT const& target) const;

	bool read(ObjectT& target, json_value_s* value) const { return (this->*read_ptr)(target, value); }
	std::string write(ObjectT const& target) const { return (this->*write_ptr)(target); }

	template<class T>
	auto member() const { return reinterpret_cast<T ObjectT::*>(member_ptr); }
	template<class T> using reduce_type =
		std::conditional_t<std::same_as<T, bool>, bool,
		std::conditional_t<std::integral<T>, int64_t,
		std::conditional_t<std::floating_point<T>, double,
		std::conditional_t<std::same_as<T, std::string>, std::string,
		std::conditional_t<std::same_as<T, std::wstring>, std::wstring,
		void>>>>>;
	template<class T>
	bool read_gen(ObjectT& target, json_value_s* value) const
	{
		using type = T::type;
		reduce_type<type> parsed{};
		if (!parse_json_value(value, parsed)) return false;
		(target.*(member<T>())).set(static_cast<type>(parsed));
		return true;
	}
	template<class T>
	std::string write_gen(ObjectT const& target) const
	{
		using type = T::type;
		return to_json_value_text(static_cast<reduce_type<type>>((target.*(member<T>())).get()));
	}

	template<class T>
	field_desc(T ObjectT::* member)
		: member_ptr{ reinterpret_cast<void* ObjectT::*>(member) }
		, read_ptr{ &field_desc::read_gen<T> }
		, write_ptr{ &field_desc::write_gen<T> } {}
};

////////////////////////////////
// field manipulation.
////////////////////////////////
std::map<std::string_view, field_desc<Setting>> const setting_fields = {
#define field(field_name)	{ #field_name, &Setting::field_name }

	field(cache_expire_minutes),
	field(cache_total_dots),

#undef field
};

bool apply_field(json_object_element_s const* element, Setting& target)
{
	if (element->name == nullptr || element->name->string == nullptr) return false;

	std::string_view const key{ element->name->string, element->name->string_size };
	auto const it = setting_fields.find(key);
	if (it == setting_fields.end()) return false;

	return it->second.read(target, element->value);
}

ANON_NS_E

////////////////////////////////
// Exported functions.
////////////////////////////////
Setting Settings::load(std::wstring const& file_path)
{
	Setting result{};
	std::string content;
	if (!read_file(file_path, content)) return result;
	if (content.empty()) return result;

	json_value_s* const root = json_parse_ex(content.data(),
		content.size(),
		json_parse_flags_allow_json5,
		nullptr, nullptr, nullptr);
	if (root == nullptr) return result;

	json_object_s* const object = json_value_as_object(root);
	if (object != nullptr) {
		for (json_object_element_s* element = object->start; element != nullptr; element = element->next)
			apply_field(element, result);
	}

	::free(root);
	return result;
}

void Settings::save(std::wstring const& file_path, Setting const& settings)
{
	std::string content = "{\n"s;
	for (auto const& [key, field] : setting_fields) {
		content += "  \""sv;
		escape_json_string_to(content, key);
		content += "\": "sv;
		content += field.write(settings);
		content += ",\n"sv;
	}
	content += "}\n"sv;
	write_file(file_path, content);
}
