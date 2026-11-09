# InlineScene_S AviUtl ExEdit2 仮想シーンプラグイン

別シーンに移動しなくても，1つのシーン内に仮想的に別のシーンを配置できるプラグインです．

標準の「画像合成(オブジェクト)」とは異なり，カメラ制御を含めたり，入れ子構造のシーンにも対応しています．

画像のキャッシュ機能とも併用しているので，重い静止画のシーンを他フレームで軽量なまま再利用することなどもできます．

AviUtl (無印) の InlineScene_S に比べて，音声にも対応しています．

[ダウンロードはこちら．](https://github.com/sigma-axis/aviutl2_InlineScene_S/releases) \[紹介動画．\](WIP)

次のオブジェクトやフィルタ効果が追加されます:

1.  [InlineScene_Sここから](#inlinescene_sここから)
1.  [InlineScene_Sここまで](#inlinescene_sここまで)
1.  [InlineScene_S次へ](#inlinescene_s次へ)
1.  [InlineScene_S読み出し](#inlinescene_s読み出し)
1.  [InlineScene_Sこのオブジェクトから](#inlinescene_sこのオブジェクトから)
1.  [InlineScene_Sこのオブジェクトで次へ](#inlinescene_sこのオブジェクトで次へ)
1.  [InlineScene_S単品保存](#inlinescene_s単品保存)

![Inline Scene S のイメージ画像](https://github.com/user-attachments/assets/f59bd11f-02ff-4c07-9da8-c9bd65678ad1)

##  お願い

このプラグインを使った動画などでは，ニコニコの親作品にこのプラグインの紹介動画を登録してくれると嬉しいです．任意ではありますが，登録してくれたほうが励みになります．

- 登録 ID: `sm****`

##  動作要件

- AviUtl ExEdit2

  http://spring-fragrance.mints.ne.jp/aviutl

  - `2.1.16` で動作確認済み．

## 導入方法

ダウンロードした `aviutl2_InlineScene_S-v*.**.au2pkg.zip` を AviUtl2 のウィンドウにドラッグ & ドロップしてください．

初期状態だと「メディアオブジェクトを追加」や「フィルタ効果を追加」メニューの「InlineScene_S」に各種オブジェクト・フィルタ効果が追加されています．
- 「オブジェクト追加メニューの設定」の「ラベル」項目で分類を変更できます．

### For non-Japanese speaking users

You may be able to find language translation file for this plugin from [this repository](https://github.com/sigma-axis/aviutl2_translations_sigma-axis). 
Translation files enable names and parameters of the scripts / filters to be displayed in other languages.

Although, usage documentations for this plugin in languages other than Japanese are not available now.

##  仕組み

[「InlineScene_Sここから」](#inlinescene_sここから)が処理されると，現在のフレームバッファの画像をバックアップとして保存，現在のフレームバッファを完全透明にクリアします．こうすることで次以降のレイヤーでは，*あたかも*新規シーンで処理されたかのように画像データがフレームバッファに描画されていくようになります．

[「InlineScene_Sここまで」](#inlinescene_sここまで)が処理されると，「InlineScene_Sここから」でバックアップされていたフレームバッファの画像を復元，今あるフレームバッファの画像を現在のオブジェクトの画像として利用します．こうすることで「InlineScene_Sここから」と「InlineScene_Sここまで」の間にあるレイヤー上のオブジェクトが1枚の画像にまとめられます．また，次以降のレイヤーでは通常通りに描画処理が行われるようになります．

また，音声に関しても同様にオーディオバッファに介入することで，別シーンのような取り扱いを実現できます．

このように，通常のオブジェクトとは違う特殊な介入操作を行います．そのため，例えば「InlineScene_Sここから」を配置したにも関わらず「InlineScene_Sここまで」を置き忘れていた (あるいはフレームずれで「InlineScene_Sここまで」が処理されなかった) 場合，背景が真っ黒なままになるなど，小さなミスが大きな影響になってしまうこともあります．

こういったミスは検知できる範囲で，ログウィンドウに[警告メッセージ](#ログ表示のメッセージ)を出力するようにしています．動画出力をした際などにはこのメッセージの確認をおすすめします．

![警告メッセージの例](https://github.com/user-attachments/assets/4081eb5f-7480-4ca8-b160-5730cc30872d)

また，本来のシーン機能では可能な早送り，一時停止などの時間操作はできません．

##  使い方

### 基本的な使い方

シーンとしてまとめたい複数レイヤーに対して，次のようにオブジェクトを配置していきます．

1.  当該複数レイヤーの1つ上に[「InlineScene_Sここから」](#inlinescene_sここから)を配置．
1.  当該複数レイヤーの1つ下に[「InlineScene_Sここまで」](#inlinescene_sここまで)を配置．
1.  そのまた1つ下に[「InlineScene_S読み出し」](#inlinescene_s読み出し)を配置．

この「InlineScene_S読み出し」のオブジェクトが複数レイヤーをまとめた画像・音声のオブジェクトとなっていて，通常のシーンオブジェクトに相当するものになります．

「InlineScene_S読み出し」で指定する「保存名」は「InlineScene_Sここまで」での「保存名」と同じものを指定してください．

![基本レイヤー配置](https://github.com/user-attachments/assets/a58dbe8c-1b1e-4d63-8214-6d93840f4cf0)

### 派生的な使い方

機能面としては[基本的な使い方](#基本的な使い方)と同じですが，レイヤー数を節約したり配置に自由度ができたりします．

- [「InlineScene_S読み出し」](#inlinescene_s読み出し)は「InlineScene_Sここまで」直下に限らず，もっと下の離れたレイヤーに置くこともできます．レイヤー配分に自由度ができます．

  - [「InlineScene_Sここまで」の「モード」](#モード)で `長期キャッシュ` を選んでいた場合，別フレームや別シーンでも再利用できます．

- 「InlineScene_Sここまで」の「モード」で「オブジェクトとして表示・再生」を選ぶと，このオブジェクトがシーンオブジェクトのように振る舞います．そのため「InlineScene_S読み出し」を省略でき，レイヤー数を節約できます．
  - 一部条件下で，この方法では正しく動作しない場面があります ([Inline Scene の制約](#inline-scene-の制約)).

- [「InlineScene_Sここまで」](#inlinescene_sここまで)と[「InlineScene_Sここから」](#inlinescene_sここから)をまとめたものとして[「InlineScene_S次へ」](#inlinescene_s次へ)が使えます．

  - レイヤー数を節約できることに加えて，内部処理的にも手順が少なく低負荷です．

  ![InlineScene_S次への配置例](https://github.com/user-attachments/assets/e00e9136-3c1f-4e62-af97-207f25843c83)

- メディアオブジェクトの[「InlineScene_Sここから」](#inlinescene_sここから)を配置する代わりに，**inline scene の最初のオブジェクト**にフィルタ効果の[「InlineScene_Sこのオブジェクトから」](#inlinescene_sこのオブジェクトから)を適用することもできます．

  - [「InlineScene_S次へ」](#inlinescene_s次へ)に対応する[「InlineScene_Sこのオブジェクトで次へ」](#inlinescene_sこのオブジェクトで次へ)もあります．
  - 一部条件下で，この方法では正しく動作しない場面があります ([Inline Scene の制約](#inline-scene-の制約)).

### 応用的な使い方

Inline scene は画像や音声を一時保存する機能も兼ねていて，同一オブジェクトを使いまわしたり，描画の重い画像のキャッシュをとって再利用することで軽量化もできます．

- [「InlineScene_S単品保存」](#inlinescene_s単品保存)

  現在オブジェクトの画像データと，座標や回転角度，透明度などの情報，そして音声データと左右の音量の情報を保存します．保存した情報は[「InlineScene_S読み出し」](#inlinescene_s読み出し)で再利用できます．

  - グループ制御オブジェクトで設定された座標移動や回転角度は保存されません．
  - 画像に関しては，[「モード」](#モード--保存名)で `長期キャッシュ` を選んでいた場合，別フレームや別シーンでも再利用できます．
  - 音声や，「モード」で `一時キャッシュ` を選んでいた場合の画像は，別フレームや別シーンに再利用することはできません．

##  オブジェクト・フィルタ効果の説明

これらのオブジェクトやフィルタ効果は，通常のような画面への画像の配置や加工，音声の再生とは違い，描画や再生の手順に介入する処理・手続きのような役割になっています．適切なタイミングでその手続きが実行されるように，レイヤー配置やフィルタ効果の配置をする必要がある点に注意してください．

また一部条件下で，これらのオブジェクトやフィルタ効果が正しく動作しない場面があります ([Inline Scene の制約](#inline-scene-の制約)).

> [!TIP]
> これらのオブジェクトやフィルタ効果は画像と音声の両方に対応しています．
> - オブジェクトで画像のみ・音声のみで inline scene 利用したい場合は，オブジェクト設定の右クリックから「オブジェクトの出力切替」で「標準描画」や「音声再生」に切り替えてください．
> - フィルタ効果については，フィルタ対象のオブジェクトの，画像のみ・音声のみ・画像と音声の両方，の場合にそのまま対応します．

### InlineScene_Sここから

Inline scene を開始するオブジェクトです．このオブジェクトから下のレイヤーが仮想的なシーンになります．[「InlineScene_Sここまで」](#inlinescene_sここまで)などで inline scene の終了レイヤーを指定します．

- フィルタ効果版の[「InlineScene_Sこのオブジェクトから」](#inlinescene_sこのオブジェクトから)も同じ効果があります．

- 終了レイヤーを指定していない inline scene がある場合，描画・再生結果が正しくないものになることがあります．

  その場合，次フレーム以降で inline scene 関連のオブジェクトが処理された際に[ログ表示にメッセージ](#inline-scene-が正しく閉じられませんでした)が表示されます．

設定パラメタはありません．

### InlineScene_Sここまで

Inline scene を終了するオブジェクトです．[「InlineScene_Sここから」](#inlinescene_sここから)などで指定したレイヤーからこのオブジェクトの上のレイヤーまでを，仮想的なシーンとして取り扱います．

この仮想的なシーンに対しては以下の使い方ができます:
- 名前を付けて保存して，下のレイヤーや他のフレーム・シーンなどで再利用する ([「InlineScene_S読み出し」](#inlinescene_s読み出し)を利用).
- 「InlineScene_Sここまで」オブジェクトにそのまま画像を表示・音声を再生し，シーンオブジェクトのように取り扱えるようにする．

####  オブジェクトとして表示・再生

ON の場合，このオブジェクトにそのまま画像を表示・音声を再生します．
- このオブジェクトがシーンオブジェクトのように取り扱えます．
- 名前を付けて保存して[「InlineScene_S読み出し」](#inlinescene_s読み出し)で呼び出す方法に比べて，1レイヤー節約できます．
- 一部条件下で，この方法では正しく動作しない場面があります ([Inline Scene の制約](#inline-scene-の制約)).

OFF の場合，画像・音声を，キャッシュとして名前を付けて保存して[「InlineScene_S読み出し」](#inlinescene_s読み出し)で再利用できるようにします．

初期値は OFF.

####  モード

画像に対して有効な項目です．[「オブジェクトとして表示・再生」](#オブジェクトとして表示再生)が OFF の場合にのみ有効な設定です．

仮想的なシーンの保存方法を指定します．次の選択肢があります:

- `一時キャッシュ`

  [「一時キャッシュ」](#一時キャッシュ)として保存します．保存した画像や音声は「InlineScene_S読み出し」で利用できます．

  詳細は[キャッシュのモード](#キャッシュのモード)を参照．

- `長期キャッシュ`

  [「長期キャッシュ」](#長期キャッシュ)として保存します．保存した画像や音声は「InlineScene_S読み出し」で利用できます．

  詳細は[キャッシュのモード](#キャッシュのモード)を参照．

初期値は `一時キャッシュ`.

####  保存名

[「オブジェクトとして表示・再生」](#オブジェクトとして表示再生)が OFF の場合にのみ有効な設定です．

シーンの画像や音声に名前を付けて保存して，[「InlineScene_S読み出し」](#inlinescene_s読み出し)で利用できるようにします．

初期値は `scene1`.

####  余白除去

画像に対して有効な項目です．

仮想的なシーンの画像の上下左右にある，完全透明ピクセルをクリッピングするかどうかを指定します．次の選択肢があります:

- `除去する`
- `除去しない`
- `上下左右個別に指定`

  直下に表示される「左」「右」「上」「下」のチェックボックスで，除去する上下左右端を個別に指定します．
  - 各チェックボックスの初期値は ON.

初期値は `除去する`.

####  追加余白

画像に対して有効な項目です．

仮想的なシーンの画像の上下左右に余白を追加します．負の値を指定するとクリッピングします．

「左」「右」「上」「下」のトラックバーで個別に指定．

最小値は -4000, 最大値は 4000, 初期値は 0.

### InlineScene_S次へ

[「InlineScene_Sここまで」](#inlinescene_sここまで)と[「InlineScene_Sここから」](#inlinescene_sここから)を組み合わせたもので，このオブジェクトの上のレイヤーまでを仮想的なシーンとして保存し，さらにその下から新しい仮想的なシーンを開始します．

「InlineScene_Sここまで」と「InlineScene_Sここから」を 2 つ配置するのに比べると，レイヤー数を節約できることに加えて，内部処理的にも手順が少なく低負荷です．

> [!NOTE]
> [「InlineScene_Sここまで」](#inlinescene_sここまで)とは異なり，[「オブジェクトとして表示・再生」](#オブジェクトとして表示再生)に対応する設定項目はありません．名前を付けて保存したうえで[「InlineScene_S読み出し」](#inlinescene_s読み出し)で再利用する前提です．

####  モード / 保存名 / 余白除去 / 追加余白

それぞれ「InlineScene_Sここまで」の[「モード」](#モード)[「保存名」](#保存名)[「余白除去」](#余白除去)[「追加余白」](#追加余白)と同じです．

### InlineScene_S読み出し

[「InlineScene_Sここまで」](#inlinescene_sここまで)や[「InlineScene_S単品保存」](#inlinescene_s単品保存)などで保存した画像や音声を復元します．

ただし画像や音声，保存元のシーンやフレームなどに応じて，次の制限があります．

- 画像の場合:

  - [「一時キャッシュ」](#一時キャッシュ)として保存した画像は，同じシーンの同じフレームでのみ復元できます．
  - [「長期キャッシュ」](#長期キャッシュ)として保存した画像は，別シーンの別フレームでも復元できます．
    - 一定時間使用しなかった「長期キャッシュ」は時間経過で削除されます．削除されるまでの時間は[設定で変更](#inlinescene_s-の設定)できます．
    - ただし，動画出力中やプレビュー再生中は自動削除が行われません．

- 音声の場合:

  同じシーンの同じフレームでのみ復元できます．

保存した時のオブジェクトに関連付けられた一部のパラメタも復元できます．
- 画像オブジェクトの場合，座標，回転角度，拡大率，回転中心，透明度が復元の対象です．
- 音声オブジェクトの場合，左右チャンネルそれぞれの音量が復元の対象です．

####  モード

画像に対して有効な項目です．復元対象の保存時の「モード」を指定します．詳細は[キャッシュのモード](#キャッシュのモード)を参照．

次の選択肢があります:
- `一時キャッシュ`
- `長期キャッシュ`

初期値は `一時キャッシュ`.

> [!IMPORTANT]
> 同じ[「保存名」](#保存名-1)でも「モード」が違うと別枠扱いです．

####  保存名

復元対象の保存時の「保存名」を指定します．

初期値は `scene1`.

### 位置や回転も復元 / 回転の合成

画像に対して有効な項目です．

- 位置や回転も復元

  復元対象の画像の，保存時での以下の項目を復元します:
  1.  座標
  1.  回転角度
  1.  拡大率
  1.  回転中心
  1.  透明度

  保存時の「標準描画」などの出力フィルタの設定と，フィルタ効果による影響の合算値を，フィルタ効果による影響の数値として復元します．

  復元しない場合，基本的に画像は中央揃えの配置になります．

  初期値は ON.

- 回転の合成

  「位置や回転も復元」が ON の場合に有効な項目です．

  回転角度の復元の際，「InlineScene_S読み出し」オブジェクトの標準描画の回転角度を加味して，「保存した回転 :arrow_right: 標準描画の回転」という合成で復元します．これにより，プレビュー画面の回転アンカーの操作が自然になります．

  初期値は ON.

### 音量設定も復元

音声に対して有効な項目です (画像のみの場合は無視されます).

復元対象の音声の保存時での，左右チャンネルそれぞれの音量を復元します．

保存時の「音声再生」の設定と，フィルタ効果による影響の合算値を，フィルタ効果による影響の数値として復元します．

初期値は ON.

### InlineScene_Sこのオブジェクトから

[「InlineScene_Sここから」](#inlinescene_sここから)のフィルタ効果版です．

このフィルタ効果を付けたオブジェクトを含めて，ここから下のレイヤーが仮想的なシーンになります．

- 画像のみのオブジェクトにつけると，画像のみの inline scene になります．
- 音声のみのオブジェクトにつけると，音声のみの inline scene になります．
- 画像と音声両方のオブジェクトだと，画像と音声両方の inline scene です．

設定パラメタはありません．

### InlineScene_Sこのオブジェクトで次へ

[「InlineScene_S次へ」](#inlinescene_s次へ)のフィルタ効果版です．

このフィルタ効果を付けたオブジェクトより上のレイヤーを仮想的なシーンとして保存，このフィルタ効果を付けたオブジェクトを含めた下のレイヤーを新しい仮想的なシーンとして開始します．

- 画像のみのオブジェクトにつけると，画像のみの inline scene になります．
- 音声のみのオブジェクトにつけると，音声のみの inline scene になります．
- 画像と音声両方のオブジェクトだと，画像と音声両方の inline scene です．

設定パラメタは「InlineScene_S次へ」と同じです．

### InlineScene_S単品保存

現在の画像や音声に名前を付けて保存し，[「InlineScene_S読み出し」](#inlinescene_s読み出し)で呼び出して再利用できるようにします．画像の拡縮回転や，音声の左右チャンネルの音量などの情報も保存します．

- 画像のみのオブジェクトにつけると，画像のみが保存の対象になります．
- 音声のみのオブジェクトにつけると，音声のみが保存の対象になります．
- 画像と音声両方のオブジェクトだと，画像と音声両方が保存の対象です．

####  モード / 保存名

「InlineScene_S読み出し」で指定する[「モード」](#モード-1)と[「保存名」](#保存名-1)を設定します．

- 「モード」の初期値は `一時キャッシュ`.
- 「保存名」の初期値は `scene1`.

####  表示・再生しない

このオブジェクトでの画像の描画や音声の再生を行いません．他オブジェクトの「InlineScene_S読み出し」で呼び出すための，保存専用のオブジェクトとします．

##  キャッシュのモード

各種オブジェクトの「モード」指定では「一時キャッシュ」と「長期キャッシュ」を選びます．これらの違いについて説明します．

> [!TIP]
> どちらのモードのキャッシュも AviUtl2 メニューの「編集」 :arrow_right: 「キャッシュを破棄」で削除できます．

> [!NOTE]
> これらは画像の保存にのみ関連し，音声の保存には影響しません．

### 一時キャッシュ

次の性質があります:
- フレーム描画ごとに破棄され，シーンごとに個別に管理されるため，別フレームや別シーンに持ち越すことができない．
- フレーム描画ごとに破棄されるため，描画処理以外では GPU メモリを圧迫しない．
- AviUtl2 のメモリ管理下に置かれているため，同じシーンの同じフレームでも自動的に削除されることがある．
  - 「一時画像キャッシュの最大値」に達すると自動削除される仕組みによるものです．これは AviUtl2 の設定ファイル `system.conf` の `TemporaryImageCacheSize` を増やすことで削除されにくくすることができます．
- スクリプトの `obj.copybuffer(..., "cache:<保存名>")` でも読み出すことができ，逆に上書きすることもできる．

AviUtl2 の他スクリプトからも利用できるという点では汎用性がありますが，逆に他スクリプトなどによって意図せぬ上書きが発生する可能性もあります．

また，別フレームや別シーンに持ち越すことができません．

### 長期キャッシュ

次の性質があります:
- InlineScene_S のプラグインが独自管理している GPU メモリに置かれるため，他スクリプトからの読み出しや上書きができない．
- シーンやフレームごとに分けずに管理しているため，別フレームや別シーンに持ち越すことができる．
- 一定時間使用しなかったキャッシュは時間経過で削除される．
  - 削除されるまでの時間は[設定で変更](#inlinescene_s-の設定)できます．
- [設定値「キャッシュの総ドット数」](#inlinescene_s-の設定)に達しても自動的に削除される．
- ただし自動削除は，動画出力中やプレビュー再生中には行われない．

別フレームや別シーンに持ち越すことができる一方，AviUtl2 の他スクリプトからは利用できません．

##  InlineScene_S の設定

メインメニューの「設定」 :arrow_right: 「プラグイン設定」 :arrow_right: 「InlineScene_S」から設定ダイアログを表示できます．

![Inline Scene S の設定ダイアログ](https://github.com/user-attachments/assets/e863d75a-be0d-4985-95ff-d62ac882855e)

次の設定項目があります:

- **長期キャッシュの保持期間 (分)**

  [「長期キャッシュ」](#長期キャッシュ)として保存された画像は，一定時間使われないと自動的に削除されます．その削除されるまでの保持期間を分単位で指定します．

  - 動画出力中やプレビュー再生中は自動削除されません．

  初期値は 2 分．最小値は 1 分，最大値は 60 分．

- **長期キャッシュの総ドット数**

  [「長期キャッシュ」](#長期キャッシュ)として保存される画像の合計サイズの最大値です．合計サイズがこの値を超えた場合，最後に使われた時刻が古い順に，合計サイズが指定値以下になるまで削除されます．

  - 1 ドット当たりのサイズは 8 バイトです (実際には各種オーバーヘッドで大きくなる傾向があります).
  - 動画出力中やプレビュー再生中は自動削除されません．

  初期値は 16 000 000．最小値は 16 000 000，最大値は 512 000 000．

##  Inline Scene の制約

一部のオブジェクトやフィルタ効果は，特定の条件で正しく動作しません．一部はログウィンドウに[警告メッセージ](#ログ表示のメッセージ)として出力されます．

### 個別オブジェクトの場合

次のフィルタ効果は個別オブジェクトだと，不定回数の複数回実行されるため動作しないようにしています:

1.  [InlineScene_Sこのオブジェクトから](#inlinescene_sこのオブジェクトから)
1.  [InlineScene_Sこのオブジェクトで次へ](#inlinescene_sこのオブジェクトで次へ)

検知した場合，[警告メッセージ](#inline-scene-の開始終了は個別オブジェクトには適用しないでください)が出力されます．

また[「InlineScene_S単品保存」](#inlinescene_s単品保存)については基本的に，個別オブジェクトの最後のオブジェクトが保存されるようになります．

### カメラ制御下の場合

次のオブジェクトやフィルタ効果はカメラ制御の配下だと，オブジェクトの Z 座標によって処理順序がレイヤー順とは異なる場合があるため，正しく動作しない可能性があります:

1.  [InlineScene_Sここから](#inlinescene_sここから)
1.  [InlineScene_Sここまで](#inlinescene_sここまで)
1.  [InlineScene_S次へ](#inlinescene_s次へ)
1.  [InlineScene_Sこのオブジェクトから](#inlinescene_sこのオブジェクトから)
1.  [InlineScene_Sこのオブジェクトで次へ](#inlinescene_sこのオブジェクトで次へ)

検知した場合，[警告メッセージ](#inline-scene-の開始終了はカメラ制御下には配置しないでください)が出力されます．

> [!IMPORTANT]
> カメラ制御自体は，inline scene 内に入れても問題ありません．
>
> ただし，inline scene の境界をまたぐようなカメラ制御の配置は避けてください．

### 「クリッピングオブジェクト」や「上のオブジェクトでクリッピング」の場合

次のオブジェクトやフィルタ効果には，「クリッピングオブジェクト」や「上のオブジェクトでクリッピング」を設定できません:

1.  [InlineScene_Sここから](#inlinescene_sここから)
1.  [InlineScene_Sここまで](#inlinescene_sここまで)
1.  [InlineScene_S次へ](#inlinescene_s次へ)
1.  [InlineScene_Sこのオブジェクトから](#inlinescene_sこのオブジェクトから)
1.  [InlineScene_Sこのオブジェクトで次へ](#inlinescene_sこのオブジェクトで次へ)

また「クリッピングオブジェクト」の直下のレイヤーや，「上のオブジェクトでクリッピング」の直上のレイヤーにも，これらのオブジェクトやフィルタ効果を配置すると想定通り動作しなくなることがあります．

- 「クリッピングオブジェクト」や「上のオブジェクトでクリッピング」を適用していたり，「クリッピングオブジェクト」の直下のレイヤーの場合，フレームバッファに対する操作の影響が一部分に限られてしまい，想定した動作をしません．
- 「上のオブジェクトでクリッピング」の直上のレイヤーの場合，オブジェクトの描画処理が 2 回実行されるため，inline scene の入れ子回数が重複してカウントされてしまいます．

「クリッピングオブジェクト」や「上のオブジェクトでクリッピング」を設定している場合は，検知して[警告メッセージ](#inline-scene-の開始終了はクリッピングオブジェクト-や-上のオブジェクトでクリッピング-を適用したりその影響下に配置しないでください)を出力します．

「クリッピングオブジェクト」の直下のレイヤーや，「上のオブジェクトでクリッピング」の直上のレイヤーに配置した場合は検知する仕組みはありません．ただし「上のオブジェクトでクリッピング」の直上のレイヤーに配置した場合，別の警告メッセージ ([終了しなかった](#inline-scene-が正しく閉じられませんでした)または[開始せずに終了](#inline-scene-が開始していない状態で終了しようとしました)) が出力されることがあります．

##  ログ表示のメッセージ

### 「指定された名前のキャッシュが見つかりません！」

[「InlineScene_S読み出し」](#inlinescene_s読み出し)で，[「モード」](#モード-1)と[「保存名」](#保存名-1)に対応したデータが見つからない場合に出力されます．

- 多くは「保存名」の入力ミスや，モードの選択が間違っているのが原因です．
- 他にも保存 :arrow_right: 復元の順序が逆転していたり，[「長期キャッシュ」](#長期キャッシュ)が時間経過で自然消滅していた場合にも出力されます．

### 「指定された名前のキャッシュは削除されています．キャッシュサイズが上限に達した可能性があります．設定ファイル "system.conf" 内の "TemporaryImageCacheSize" を増やしてみてください．」

次のオブジェクトやフィルタ効果で使用する，一時画像のキャッシュが予期せず削除されていた場合に出力されます:

1.  [InlineScene_Sここまで](#inlinescene_sここまで)
1.  [InlineScene_S次へ](#inlinescene_s次へ)
1.  [InlineScene_S読み出し](#inlinescene_s読み出し)
1.  [InlineScene_Sこのオブジェクトで次へ](#inlinescene_sこのオブジェクトで次へ)

多くは AviUtl2 の設定の「一時画像キャッシュの最大値」に引っかかって自動削除されたのが原因です．設定ファイル `system.conf` 内の `TemporaryImageCacheSize` を増やしてみてください．
- 実行ファイル `aviutl2.exe` と同階層に `system.conf` がありますが，これを直接編集するのではなく，アプリケーションデータフォルダ (通常は `C:\ProgramData\aviutl2`) にコピーを取って，そのコピーを編集するのを推奨します．
  - 「アプリケーションデータフォルダ」は AviUtl2 メインメニューの「その他」 :arrow_right: 「アプリケーションデータ」 :arrow_right: 「アプリケーションデータフォルダ」から開けます．
- この数値の初期値は `16000000` です ($4000 \times 4000$ の画像相当).


### 「指定された名前のキャッシュは削除されています．キャッシュサイズが上限に達しました．設定 → プラグイン設定 → InlineScene_S から "キャッシュの総ドット数" を増やしてみてください．」

[「InlineScene_S読み出し」](#inlinescene_s読み出し)で，[「モード」](#モード-1)が `長期キャッシュ` で保存したものを復元しようとしたときに出力されることがあります．

`長期キャッシュ` で保存したものが多くなりすぎて，[設定の「キャッシュの総ドット数」](#inlinescene_s-の設定)に達してしまったのが原因です．設定値を増やしてみてください．

### 「復元用のフレームバッファのキャッシュに予期せぬ変更が加えられていました！」

[「InlineScene_Sここまで」](#inlinescene_sここまで)で出力される可能性があります．

[仕組み](#仕組み)にもあるように，[「InlineScene_Sここから」](#inlinescene_sここから)ではフレームバッファのバックアップを取り，そのバックアップをのちに復元します．一方でこのバックアップは，他スクリプトなどからも介入が可能な形で保存されています．他スクリプトからの介入の結果，復元できる形になっていなかった場合に出力されます．

このメッセージにキャッシュ名も同時に出力されています．本来意図せず重複するはずのない名前ですが，この名前を使っているスクリプトなどがないかどうかを調べてください．

### 「Inline Scene が開始していない状態で終了しようとしました！」

次のオブジェクトやフィルタ効果で，inline scene が 1 つも開始していない状態のときに出力されます:

1.  [InlineScene_Sここまで](#inlinescene_sここまで)
1.  [InlineScene_S次へ](#inlinescene_s次へ)
1.  [InlineScene_Sこのオブジェクトで次へ](#inlinescene_sこのオブジェクトで次へ)

多くは[「InlineScene_Sここから」](#inlinescene_sここから)の置き忘れや，「InlineScene_Sここから」と「InlineScene_Sここまで」とのフレームずれが原因です．メッセージ元のオブジェクトや，対応する「InlineScene_Sここから」の位置や長さを確認してください．

### 「Inline Scene が正しく閉じられませんでした！」

[「InlineScene_Sここまで」](#inlinescene_sここまで)が足りずにフレームの描画・再生処理が終わった後，次以降の描画・再生処理で inline scene の開始・終了処理がされたときに出力されます．

多くは「InlineScene_Sここまで」の置き忘れや，[上述のメッセージ](#inline-scene-が開始していない状態で終了しようとしました)と同様，フレームずれが原因です．Inline scene の開始・終了オブジェクトの開始位置や長さを確認してください．

### 「Inline Scene の開始終了は，個別オブジェクトには適用しないでください！」

次のフィルタ効果が個別オブジェクトで処理されたときに出力されます:

1.  [InlineScene_Sこのオブジェクトから](#inlinescene_sこのオブジェクトから)
1.  [InlineScene_Sこのオブジェクトで次へ](#inlinescene_sこのオブジェクトで次へ)

[Inline Scene の制約](#個別オブジェクトの場合)にもあるように，個別オブジェクトにはこれらのフィルタ効果は適用しないでください．

### 「Inline Scene の開始終了は，カメラ制御下には配置しないでください！」

次のオブジェクトやフィルタ効果が，カメラ制御下にあった場合に出力されます:

1.  [InlineScene_Sここから](#inlinescene_sここから)
1.  [InlineScene_Sここまで](#inlinescene_sここまで)
1.  [InlineScene_S次へ](#inlinescene_s次へ)
1.  [InlineScene_Sこのオブジェクトから](#inlinescene_sこのオブジェクトから)
1.  [InlineScene_Sこのオブジェクトで次へ](#inlinescene_sこのオブジェクトで次へ)

[Inline Scene の制約](#カメラ制御下の場合)にもあるように，inline scene の境界をまたぐようなカメラ制御の配置は避けてください．

### 「Inline Scene の開始終了は，"クリッピングオブジェクト" や "上のオブジェクトでクリッピング" を適用したり，その影響下に配置しないでください！」

次のオブジェクトやフィルタ効果に，「クリッピングオブジェクト」や「上のオブジェクトでクリッピング」が適用されていた場合に出力されます:

1.  [InlineScene_Sここから](#inlinescene_sここから)
1.  [InlineScene_Sここまで](#inlinescene_sここまで)
1.  [InlineScene_S次へ](#inlinescene_s次へ)
1.  [InlineScene_Sこのオブジェクトから](#inlinescene_sこのオブジェクトから)
1.  [InlineScene_Sこのオブジェクトで次へ](#inlinescene_sこのオブジェクトで次へ)

[Inline Scene の制約](#クリッピングオブジェクトや上のオブジェクトでクリッピングの場合)にもあるように，「クリッピングオブジェクト」や「上のオブジェクトでクリッピング」があると inline scene の開始終了処理が正しく動作しません．

同様に，これらの影響下 (「クリッピングオブジェクト」の直下のレイヤーや，「上のオブジェクトでクリッピング」の直上のレイヤー) にも inline scene の開始終了処理は配置しないでください (こちらは警告メッセージは出力されません).

## TIPS

1.  Inline scene は入れ子にできます --- inline scene の中にまた「子 inline scene」を配置することもできます．

1.  Inline scene の中にはシーンチェンジも配置することができます．


##  改版履歴

- **v1.00** (2026-11-09)

  - 初版．

## ライセンス

このプログラムの利用・改変・再頒布等に関しては MIT ライセンスに従うものとします．

---

The MIT License (MIT)

Copyright (C) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

https://mit-license.org/


# Credits

##  AviUtl ExEdit2 SDK

http://spring-fragrance.mints.ne.jp/aviutl

```
---------------------------------
AviUtl ExEdit2 Plugin SDK License
---------------------------------

The MIT License

Copyright (c) 2025 Kenkun

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

## DirectXMathのライセンス

```
MIT License

Copyright (c) Microsoft Corporation.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE
```

#  連絡・バグ報告

- GitHub: https://github.com/sigma-axis
- Twitter: https://x.com/sigma_axis
- nicovideo: https://www.nicovideo.jp/user/51492481
- Misskey.io: https://misskey.io/@sigma_axis
- Bluesky: https://bsky.app/profile/sigma-axis.bsky.social
