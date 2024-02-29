# Reactive Dialog AviUtl プラグイン

拡張編集の設定ダイアログへのデータ入力・調整・操作方法を拡充するプラグインです．

[ダウンロードはこちら．](https://github.com/sigma-axis/aviutl_reactive_dlg/releases)

## 機能紹介

以下の機能があります．各機能は[設定ファイル](#設定ファイルについて)で個別に有効化 / 無効化できます．

1. テキスト入力ボックスから `TAB` キーでフォーカスを移動する機能．[[詳細](#テキスト入力ボックスからフォーカス移動)]

   ![テキストボックス間でフォーカス移動](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/6f99656b-ce7d-4b2c-820d-f097a3906a5d)

   - 設定で `TAB` キー以外のキーに割り当てることもできます．
   - 相補的に，ショートカットキーでテキスト入力ボックスにフォーカスを移動するコマンドも追加します．

1. トラックバーの数値入力ボックスにもショートカットキーでフォーカスを移動するコマンドを追加．[[詳細](#トラックバーの数値入力ボックスにフォーカス移動)]

1. トラックバーの数値入力ボックス上での :arrow_up: / :arrow_down: キー入力で数値を操作できる機能．[[詳細](#トラックバーの数値入力ボックスで上下キーを押して数値変更)]

   ![上下キーで数値操作](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/a56fc200-31e7-4f77-96bd-c899edd87c1c)

   - `Ctrl` / `Shift` / `Alt` キーと組み合わせて小数点以下の最小単位や10倍移動などもできます．
   - `ESC` キーで編集前の値に戻します．既に編集前の値ならフォーカスを外します．

1. トラックバー付近での `Ctrl` + マウスホイールで数値を操作できる機能．[[詳細](#トラックバー付近でマウスホイールで数値を操作)]

   ![Ctrl+マウスホイールで数値操作](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/b3600538-b0ba-4e4e-bbff-b6bb0957a181)

   - `Ctrl` / `Shift` / `Alt` キーと組み合わせて小数点以下の最小単位や10倍移動などもできます．
   - 同時押しに使う `Ctrl` キーも含めて，設定でキー割り当ての変更ができます．

1. トラックバー横の :arrow_backward: / :arrow_forward: ボタンを押したときの移動量を修飾キーで調整できる機能．[[詳細](#トラックバー横の-arrow_backward-arrow_forward-ボタンの増減量を調整)]

   ![ボタン操作に修飾キーを使えるように](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/e360a09c-aa10-4250-a567-783605235172)

   - `Ctrl` / `Shift` / `Alt` キーと組み合わせて小数点以下の最小単位や10倍移動などもできます．
   - nanypoco 様の [updown プラグイン](https://github.com/nanypoco/updown)とほぼ同等の機能です．実装手法を含め参考にさせていただきました．若干カスタマイズの自由度が異なります．

1. テキスト入力ボックスにIME入力などで複数文字を同時に入力した場合，1文字ずつ追加されてその度に画面が更新されるのを，一括で画面更新する機能．[[詳細](#テキスト入力ボックスでのime入力したときの画面更新を一括にする機能)]

   https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/78c14923-c361-4da7-8721-5b6e8c19d708

   - hebiiro 様の[エディットボックス最適化プラグイン](https://github.com/hebiiro/AviUtl-Plugin-OptimizeEditBox)で実装されていたものと類似の機能です（現在は機能削除されています）．不具合修正や実装方式を見直すなどして搭載しました．

1. テキスト入力ボックスでの `TAB` 文字の幅を調整できる機能．[[詳細](#テキスト入力ボックスでの-tab-文字の幅を調整できる機能)]

   ![TAB文字幅の変更](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/09c8ca94-b099-465a-872e-1ee7cd0dfa60)

1. フォント選択などのドロップダウンリストから名前をキー入力して項目を選べる機能．[[詳細](#ドロップダウンリストから名前のキー入力で項目選択できる機能)]

   https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/27d72205-ed26-4df3-b98c-37eeda60753e

## 動作要件

- AviUtl 1.10 + 拡張編集 0.92

  http://spring-fragrance.mints.ne.jp/aviutl
  - 拡張編集 0.93rc1 等の他バージョンでは動作しません．

- Visual C++ 再頒布可能パッケージ（\[2015/2017/2019/2022\] の x86 対応版が必要）

  https://learn.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist

## 導入方法

以下のフォルダのいずれかに `reactive_dlg.auf` と `reactive_dlg.ini` をコピーしてください．

1. `aviutl.exe` のあるフォルダ
1. (1) のフォルダにある `plugins` フォルダ
1. (2) のフォルダにある任意のフォルダ

## 使い方

### テキスト入力ボックスからフォーカス移動

テキストオブジェクトやスクリプト制御のテキスト入力ボックスにフォーカスがあるとき，`TAB` や `Shift + TAB` でタイムラインウィンドウや他のテキスト入力ボックスにフォーカスが移動します．

- テキストオブジェクトにスクリプト制御がついていて複数入力ボックスがある場合， `TAB` で上から下へ， `Shift + TAB` で下から上へ移動します．フォーカス移動先がもうない場合，タイムラインウィンドウにフォーカスが移ります．

> [!TIP]
> `TAB` と `Shift + TAB` のキーコマンドは[設定ファイル](#textboxfocus)で変更ができます．

加えて「テキストにフォーカス移動」「テキストにフォーカス移動(逆順)」というコマンドも追加しています：

- **テキストにフォーカス移動**

  テキストオブジェクトやスクリプト制御のテキストボックスにフォーカスを移動します．複数ある場合，上の方にあるものが優先します．

- **テキストにフォーカス移動(逆順)**

  「テキストにフォーカス移動」と同じですが，下の方にあるものが優先して選ばれる点が異なります．

これらにショートカットキーを割り当てれば，キーボード操作だけで，テキストボックスの編集を開始 → 中断 → データ保存 → 再開とできるようになります．

### トラックバーの数値入力ボックスにフォーカス移動

次のコマンドを追加しています：

- **トラックバーにフォーカス移動**

  トラックバー横の数値編集ボックスにフォーカスを移動します．一番上・左にあるものが優先されます．

- **トラックバーにフォーカス移動(逆順)**

  「トラックバーにフォーカス移動」と同じですが，一番下・右にあるものが優先される点が異なります．

これらの数値入力ボックスからフォーカスを外すのは `Enter` でキーボード操作から可能ですが，逆にフォーカスを移すのはマウス操作が必要だったため追加しました．お好きなショートカットキーを割り当ててください．

### トラックバーの数値入力ボックスで上下キーを押して数値変更

トラックバー横の数値入力ボックスにフォーカスがあるとき，上下キーで `±1` ずつ数値を増減できます．増減量も修飾キーで以下のように調節できます：

- `Shift` キー

  小数点以下の最小単位で増減します．
  - 例えば，拡大率だと `±0.01` ずつ増減，座標だと `±0.1` ずつ増減するようになります．整数値のみ受け付けるような箇所は `±1` のままです．

- `Alt` キー

  増減量が 10 倍になります．`Shift` キーと組み合わせることもできます．

> [!TIP]
> 修飾キーは `Ctrl`, `Shift`, `Alt` の3つの組み合わせとして[設定ファイル](#trackkeyboard)で変更ができます．また，倍率の「10倍」も変更できます．

加えて `ESC` キーで編集直前の値に戻るようにもなります．
- 既に元の値のままならフォーカスを外します．

### トラックバー付近でマウスホイールで数値を操作

トラックバー付近にマウスカーソルがある状態で `Ctrl` キーを押すとカーソルの形が変わり，その状態でホイール入力をすると，数値が `±1` ずつ増減します．増減量も修飾キーで以下のように調節できます：

- `Shift` キー

  小数点以下の最小単位で増減します．
  - 例えば，拡大率だと `±0.01` ずつ増減，座標だと `±0.1` ずつ増減するようになります．整数値のみ受け付けるような箇所は `±1` のままです．

- `Alt` キー

  増減量が 10 倍になります．`Shift` キーと組み合わせることもできます．

> [!TIP]
> 修飾キーは `Ctrl`, `Shift`, `Alt` の3つの組み合わせとして[設定ファイル](#trackmouse)で変更ができます．倍率の「10倍」も変更できます．また，`Ctrl` 等のキーを押さなくてもホイール回転だけで増減できるようにも設定できます．

### トラックバー横の :arrow_backward: :arrow_forward: ボタンの増減量を調整

トラックバー横にある :arrow_backward: / :arrow_forward: ボタンは通常は小数点以下の最小単位で増減しますが，その増減量を修飾キーで調整できます：

- `Shift` キー

  `±1` ずつ増減するようになります．
  - 例えば，拡大率だと `±0.01` ずつだったのが整数単位で増減するようになります．

- `Alt` キー

  増減量が 10 倍になります．`Shift` キーと組み合わせることもできます．

> [!TIP]
> 修飾キーは `Ctrl`, `Shift`, `Alt` の3つの組み合わせとして[設定ファイルで変更](#trackbutton)ができます．倍率の「10倍」も変更できます．

この機能は nanypoco 様の [updown プラグイン](https://github.com/nanypoco/updown)のものとほぼ同じです． *逆に言えば，このプラグインと**競合**するため updown プラグインを検知した場合この機能は無効になります．* 設定でカスタマイズ可能な項目などが微妙に異なるため使いやすい方をお選びください．

### テキスト入力ボックスでのIME入力したときの画面更新を一括にする機能

テキストオブジェクトの入力で長めの日本語文章を変換して確定したとき，AviUtl 標準だと1文字ずつ追加されてその度にプレビュー画面を更新する挙動でした．重いシーンでうっかり長い文章を確定してしまった場合，例えば 30 文字入力確定すると 30 フレーム分の描画処理が終わるまで待機する必要がありました．

画面更新の一括機能を有効にすると，1文字ずつではなく全ての文字が追加されてからプレビュー画面の更新が入るようになります．これで 100 文字入力しようが 1 フレーム分の画面更新だけで済むようになり，うっかり長時間待機してしまうようなことになりにくくなります．

- [設定ファイル](#textboxtweaks)でこの機能の有効化・無効化を切り替えられます．
- この機能は元々 hebiiro 様の[エディットボックス最適化プラグイン](https://github.com/hebiiro/AviUtl-Plugin-OptimizeEditBox)で実装されていたものと類似の機能ですが，最新版では削除されていて，後継の[アルティメットプラグイン](https://github.com/hebiiro/anti.aviutl.ultimate.plugin)の [EditBoxTweaker](https://github.com/hebiiro/anti.aviutl.ultimate.plugin/wiki/EditBoxTweaker) にも未搭載です．
  - [SplitWindow](https://github.com/hebiiro/AviUtl-Plugin-SplitWindow) との併用などで不具合があったため，修正・改変して搭載しています．後継の[アルティメットプラグイン](https://github.com/hebiiro/anti.aviutl.ultimate.plugin)の [Nest](https://github.com/hebiiro/anti.aviutl.ultimate.plugin/wiki/Nest) との共存も確認できています．
  - 私の[エディットボックス最適化プラグイン改造版](https://github.com/sigma-axis/AviUtl-Plugin-OptimizeEditBox)にも搭載していましたが，そちらは [EditBoxTweaker](https://github.com/hebiiro/anti.aviutl.ultimate.plugin/wiki/EditBoxTweaker) と競合します．方式も少し変更しています．

### テキスト入力ボックスでの `TAB` 文字の幅を調整できる機能

テキストオブジェクトとスクリプト制御のテキスト入力ボックスの `TAB` 文字幅を，Windows 標準の `32` という値から変更できます．[設定ファイルを編集](#textboxtweaks)して変更してください．

- テキストオブジェクト，スクリプト制御それぞれで個別の値を設定できます．
- こちらも私の[エディットボックス最適化プラグイン改造版](https://github.com/sigma-axis/AviUtl-Plugin-OptimizeEditBox)で搭載していた機能ですが移植しました．

### ドロップダウンリストから名前のキー入力で項目選択できる機能

テキストオブジェクトのフォント指定や，アニメーション効果のスクリプトを選ぶ際のドロップダウンリストで，選びたいものの名前がわかっている場合，キーボード入力で名前の先頭を入力するとその項目に飛べるようになります．

- 元来 Combo Box のコントロールが固有で持っている機能でしたが，AviUtl の特殊事情のためかきちんと働いていなかったのを再調整しました．

  いくつか制約があります:
  1. キーボード入力を受け付けるのはドロップダウンリストを開いている間のみです．フォーカスが当たっているだけの状態では機能しません．
  1. ドロップダウンリストを開いている間は AviUtl で設定したショートカットキーは機能しません．
  1. 半角英数の文字入力しか受け付けません（Windows 標準の仕様をそのまま引き継いでいるため）．

- [設定ファイル](#dropdownkeyboard)でこの機能の有効化・無効化を切り替えられます．

## 設定ファイルについて

テキストエディタで `reactive_dlg.ini` を編集することで挙動をカスタマイズできます．ファイル内にも説明が書いてあるためそちらもご参照ください．

### `[TextBox.Focus]`

[テキスト入力ボックスのフォーカス移動操作](#テキスト入力ボックスからフォーカス移動)に関する設定を変更できます．
- `forward.vkey` と `forward.mkeys` の組み合わせでフォーカス移動のショートカットキーを設定できます．
  - `forward.vkey` では「仮想キーコード」を指定します．\[[一覧はこちら](https://learn.microsoft.com/ja-jp/windows/win32/inputdev/virtual-key-codes)\]

    例えば `TAB` キーを指定したい場合，「値」に記載されているのが `0x09` なので `forward.vkey=0x09` のように指定します．また，`0` を指定することで無効化できます．

  - `forward.mkeys` で修飾キーの組み合わせを指定します．

    `ctrl`, `shift`, `alt` の3つを `+` で挟んで記述します（順不同）．修飾キーなしを指定する場合 `none` と記述します．
    ```ini
    ; 修飾キーなし．
    forward.mkeys=none

    ; Ctrl キー．
    forward.mkeys=ctrl

    ; Ctrl キーと Shift キー．
    forward.mkeys=ctrl+shift

    ; '+' 前後の半角空白は無視されます．大文字小文字の違いも無視されます．
    forward.mkeys=Alt + SHIFT + cTrL
    ```

  初期設定だと以下の通りです:
  ```ini
  forward.vkey=0x09
  forward.mkeys=none
  ```
  - 修飾キーなしの `TAB` キーの指定です．

- `backward.vkey` と `backward.mkeys` で「逆順」のフォーカス移動のショートカットキーを設定できます．
  
  `forward.****` との違いは，テキストオブジェクトにスクリプト制御が付いていて複数の入力ボックスがある場合，下から上のテキストボックスにフォーカス移動するという点です（`forward.****` だと上から下）．

  初期設定だと以下の通りです:
  ```ini
  backward.vkey=0x09
  backward.mkeys=shift
  ```
  - `Shift + TAB` の指定です.

### `[TextBox.Tweaks]`

テキスト入力ボックス動作を微調整します．

- `batch` で[IME入力時の画面更新を一括する機能](#テキスト入力ボックスでのime入力したときの画面更新を一括にする機能)を有効化・無効化できます．

  `0` で無効，`1` で有効です．初期設定だと `batch=1` で有効．

- `tabstops_text` と `tabstops_script` で[入力ボックスの TAB 文字の幅](#テキスト入力ボックスでの-tab-文字の幅を調整できる機能)を指定できます．初期設定だと両方とも `-1` （Windows のデフォルト設定を利用）が指定されています．
  - `-1` を指定するとデフォルト値のままです（このプラグインによる介入もしません）．
  - Windows のデフォルト値は `32` です．

### `[Track.Keyboard]`

トラックバーの数値入力ボックスでの一部キーボード操作を設定します．

- `updown` で [:arrow_up: / :arrow_down: キーによる数値操作](#トラックバーの数値入力ボックスで上下キーを押して数値変更)を有効化・無効化できます．

  `0` で無効，`1` で有効です．初期設定だと `updown=1` で有効．

  - `keys_decimal` で指定する修飾キーで，操作する桁を整数部分か，小数点以下の最小桁か切り替えられます．
    
    `ctrl`, `shift`, `alt` の組み合わせを `+` で区切って指定．

  - `def_decimal` を有効にすると，`keys_decimal` で指定の修飾キーを押していない状態での操作対象の桁が小数点以下の最小桁になります．逆に `keys_decimal` を押している状態だと整数部分の桁を操作します．

    `0` で無効，`1` で有効です．初期設定だと `def_decimal=0` で無効．

  - `keys_boost` で指定する修飾キーを押しながらだと `rate_boost` で指定された倍率で動きます．
  - `rate_boost` は `keys_boost` を押している状態での倍率を指定します．
  - `updown_clamp` を有効にしていると最大値・最小値の範囲を超えた場合，自動的にその範囲に収めるようにします．

  初期設定は以下の通りです．
  ```ini
  updown=1
  keys_decimal=shift
  keys_boost=alt
  def_decimal=0
  rate_boost=10
  updown_clamp=0
  ```
  - :arrow_up: :arrow_down: キーで整数単位の移動．
  - `Shift` キーで小数点以下の最小単位で移動．
  - `Alt` キーで 10 倍移動．`Shift+Alt` で小数点以下の最小単位の 10 倍．
  - 最大・最小の範囲には自動的に収めない．

- `escape` を有効にすると `ESC` キーを押したときに，数値入力ボックスを編集前（フォーカスを取得したタイミング）の値に戻します．既に元の値の場合はフォーカスを外します．

  `0` で無効，`1` で有効です．初期設定だと `escape=1` で有効．

  - `ESC` キーは固定です．他のキーに割り当てるなどの設定はできません．

### `[Track.Mouse]`

[設定ダイアログのトラックバーの数値をマウスホイール操作で動かせる機能](#トラックバー付近でマウスホイールで数値を操作)の設定ができます．

- `wheel` でこの機能を有効化・無効化できます．

    `0` で無効，`1` で有効です．初期設定だと `wheel=1` で有効．

- `keys_activate` で指定する修飾キーを押している間だけホイール操作を受け付けるようになります．

  `ctrl`, `shift`, `alt` の組み合わせを `+` で区切って指定．
  - `none` を指定すると修飾キーなしでホイール操作ができるようになります．

- `reverse_wheel` でホイール操作を逆転できます．`0` で既定値，`1` で逆転です．

- `keys_decimal` で指定する修飾キーで，操作する桁を整数部分か，小数点以下の最小桁か切り替えられます．
- `def_decimal` を有効にすると，`keys_decimal` で指定の修飾キーを押していない状態での操作対象の桁が小数点以下の最小桁になります．逆に `keys_decimal` を押している状態だと整数部分の桁を操作します．

  `0` で無効，`1` で有効です．初期設定だと `def_decimal=0` で無効．

- `keys_boost` で指定する修飾キーを押しながらだと `rate_boost` で指定された倍率で動きます．
- `rate_boost` は `keys_boost` を押している状態での倍率を指定します．
- `cursor_react` を有効にすると，ホイール操作が可能な場合にマウスカーソルの形状が変化します．ホイールによる誤操作を未然に防ぐなどの効果が期待できます．

  `0` で無効，`1` で有効です．初期設定だと `cursor_react=1` で有効．

初期設定は以下の通りです．
```ini
wheel=1
keys_activate=ctrl
reverse_wheel=0
keys_decimal=shift
keys_boost=alt
def_decimal=0
rate_boost=10
cursor_react=1
```
- `Ctrl` を押しながらマウスホイールで整数単位の移動．
- ホイール方向は既定値．
- `Shift` キーで小数点以下の最小単位で移動．
- `Alt` キーで 10 倍移動．`Shift+Alt` で小数点以下の最小単位の 10 倍．
- カーソル位置が対象範囲でかつ `Ctrl` キーが押されている状態だとカーソル形状が変化．

### `[Track.Button]`

[トラックバー横の :arrow_backward: :arrow_forward: をクリックしたときの増減量を修飾キーで調整する機能](#トラックバー横の-arrow_backward-arrow_forward-ボタンの増減量を調整)の設定ができます．

- `modify` でこの機能を有効化・無効化できます．

  `0` で無効，`1` で有効です．初期設定だと `modify=1` で有効．

- `keys_decimal` で指定する修飾キーで，操作する桁を整数部分か，小数点以下の最小桁か切り替えられます．

  `ctrl`, `shift`, `alt` の組み合わせを `+` で区切って指定．

- `def_decimal` を有効にすると，`keys_decimal` で指定の修飾キーを押していない状態での操作対象の桁が小数点以下の最小桁になります．逆に `keys_decimal` を押している状態だと整数部分の桁を操作します．

  `0` で無効，`1` で有効です．初期設定だと `def_decimal=1` で有効．

- `keys_boost` で指定する修飾キーを押しながらだと `rate_boost` で指定された倍率で動きます．
- `rate_boost` は `keys_boost` を押している状態での倍率を指定します．


初期設定は以下の通りです．
```ini
modify=1
keys_decimal=shift
def_decimal=1
keys_boost=alt
rate_boost=10
```
- :arrow_backward: :arrow_forward: をクリックで小数点以下の最小単位で移動．
- `Shift` キーを押しながらだと整数単位で移動．
- `Alt` キーで小数点以下の最小単位の 10 倍移動．`Shift+Alt` で 10 ずつ移動．

### `[Dropdown.Keyboard]`

[フォント選択などのドロップダウンリストから名前のキー入力で選べる機能](#ドロップダウンリストから名前のキー入力で項目選択できる機能)の有効，無効を切り替えられます．

- `search` が `0` のとき無効， `1` で有効になります．初期値だと `search=1` で有効．

## 謝辞

- [テキスト入力時のプレビュー画面一括更新の機能](#テキスト入力ボックスでのime入力したときの画面更新を一括にする機能)は，アイデア，実装方法を含めて hebiiro 様の[エディットボックス最適化プラグイン](https://github.com/hebiiro/AviUtl-Plugin-OptimizeEditBox)を参考にさせていただきました．このような場で恐縮ですが大変便利なプラグインの作成・公開に感謝申し上げます．

- [トラックバー横の :arrow_backward: :arrow_forward: をクリックしたときの増減量を修飾キーで調整する機能](#トラックバー横の-arrow_backward-arrow_forward-ボタンの増減量を調整)は，アイデア，実装方法を含めて nanypoco 様の [updown プラグイン](https://github.com/nanypoco/updown)を参考にさせていただきました．この場ようなで恐縮ですが大変便利なプラグインの作成・公開に感謝申し上げます．

## 改版履歴

- **v1.00** (2024-02-28)

  - 初版．

## ライセンス

このプログラムの利用・改変・再頒布等に関しては MIT ライセンスに従うものとします．

---

The MIT License (MIT)

Copyright (C) 2024 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

https://mit-license.org/


#  Credits

##  aviutl_exedit_sdk

https://github.com/ePi5131/aviutl_exedit_sdk

---

1条項BSD

Copyright (c) 2022
ePi All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
THIS SOFTWARE IS PROVIDED BY ePi “AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ePi BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

##  AviUtl プラグイン - エディットボックス最適化

https://github.com/hebiiro/AviUtl-Plugin-OptimizeEditBox

---

MIT License

Copyright (c) 2021 hebiiro

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
SOFTWARE.

##  updown

https://github.com/nanypoco/updown

---

MIT License

Copyright (c) 2023 nanypoco

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
SOFTWARE.

#  連絡・バグ報告

- GitHub: https://github.com/sigma-axis
- Twitter: https://twitter.com/sigma_axis
- nicovideo: https://www.nicovideo.jp/user/51492481
- Misskey.io: https://misskey.io/@sigma_axis
- Bluesky: https://bsky.app/profile/sigma-axis.bsky.social
