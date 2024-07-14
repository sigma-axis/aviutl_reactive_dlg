# Reactive Dialog AviUtl プラグイン

拡張編集の設定ダイアログへのデータ入力・調整・操作方法を拡充するプラグインです．

[ダウンロードはこちら．](https://github.com/sigma-axis/aviutl_reactive_dlg/releases) [紹介動画．](https://www.nicovideo.jp/watch/sm43602590)

## 機能紹介

以下の機能があります．各機能は[設定ファイル](#設定ファイルについて)で個別に有効化 / 無効化できます．

1.  テキスト入力ボックスから `TAB` キーでフォーカスを移動する機能．[[詳細](#テキスト入力ボックスからフォーカス移動)]

    ![テキストボックス間でフォーカス移動](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/6f99656b-ce7d-4b2c-820d-f097a3906a5d)

    - 設定で `TAB` キー以外のキーに割り当てることもできます．
    - 相補的に，ショートカットキーでテキスト入力ボックスにフォーカスを移動するコマンドも追加します．

1.  トラックバーの数値入力ボックスにもショートカットキーでフォーカスを移動するコマンドを追加．[[詳細](#トラックバーの数値入力ボックスにフォーカス移動)]

1.  トラックバーの数値入力ボックス上での :arrow_up: / :arrow_down: キー入力で数値を操作できる機能．[[詳細](#トラックバーの数値入力ボックスで上下キーを押して数値変更)]

    ![上下キーで数値操作](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/a56fc200-31e7-4f77-96bd-c899edd87c1c)

    - `Ctrl` / `Shift` / `Alt` キーと組み合わせて小数点以下の最小単位や10倍移動などもできます．
    - `ESC` キーで編集前の値に戻します．既に編集前の値ならフォーカスを外します．

1.  トラックバー付近での `Ctrl` + マウスホイールで数値を操作できる機能．[[詳細](#トラックバー付近でマウスホイールで数値を操作)]

    ![Ctrl+マウスホイールで数値操作](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/b3600538-b0ba-4e4e-bbff-b6bb0957a181)

    - `Ctrl` / `Shift` / `Alt` キーと組み合わせて小数点以下の最小単位や10倍移動などもできます．
    - 同時押しに使う `Ctrl` キーも含めて，設定でキー割り当ての変更ができます．

1.  トラックバーの数値入力ボックスのドラッグでマウス位置を固定する機能．[[詳細](#トラックバーの数値入力ボックスのドラッグでマウス位置を固定する機能)]

    ![ドラッグしてもマウス位置固定](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/6bb074db-0a38-49e6-ba9f-095f131272e2)

    - このデモから更にバージョンが上がってカーソルが非表示になりました．

    物理的な机が続く限りドラッグし続けられます．

1.  トラックバー横の :arrow_backward: / :arrow_forward: ボタンを押したときの移動量を修飾キーで調整できる機能．[[詳細](#トラックバー横の-arrow_backward-arrow_forward-ボタンの増減量を調整)]

    ![ボタン操作に修飾キーを使えるように](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/e360a09c-aa10-4250-a567-783605235172)

    - `Ctrl` / `Shift` / `Alt` キーと組み合わせて小数点以下の最小単位や10倍移動などもできます．
    - nanypoco 様の [updown プラグイン](https://github.com/nanypoco/updown)とほぼ同等の機能です．実装手法を含め参考にさせていただきました．若干カスタマイズの自由度が異なります．

1.  テキスト入力ボックスにIME入力などで複数文字を同時に入力した場合，1文字ずつ追加されてその度に画面が更新されるのを，一括で画面更新する機能．[[詳細](#テキスト入力ボックスでのime入力したときの画面更新を一括にする機能)]

    https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/78c14923-c361-4da7-8721-5b6e8c19d708

    - hebiiro 様の[エディットボックス最適化プラグイン](https://github.com/hebiiro/AviUtl-Plugin-OptimizeEditBox)で実装されていたものと類似の機能です（現在は機能削除されています）．不具合修正や実装方式を見直すなどして搭載しました．

1.  テキスト入力ボックスでの文字入力中にマウスカーソルを自動的に非表示にする機能．[[詳細](#テキスト入力ボックスに文字を入力中はマウスカーソルを非表示にする機能)]

    カーソルが文字に被って邪魔なので手の甲でマウスを撥ね除けるような，お行儀の悪いことをしなくてもよくなります．

1.  テキスト入力ボックスでの `TAB` 文字の幅を調整できる機能．[[詳細](#テキスト入力ボックスでの-tab-文字の幅を調整できる機能)]

    ![TAB文字幅の変更](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/09c8ca94-b099-465a-872e-1ee7cd0dfa60)

1.  テキスト入力ボックスで `TAB` 文字を入力した際，代わりに予め指定した別の文字列を入力する機能．[[詳細](#テキスト入力ボックスでの-tab-文字入力を別の文字列に置き換える機能)]

    テキストオブジェクト内での `TAB` 文字はあってないような扱いなので，例えば半角空白 4 個だったり制御文字 `<p+80,+0>`（80 ピクセルのカーニング）など有意義な文字列に置き換えられます．

1.  フォント選択などのドロップダウンリストから名前をキー入力して項目を選べる機能．[[詳細](#ドロップダウンリストから名前のキー入力で項目選択できる機能)]

    https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/27d72205-ed26-4df3-b98c-37eeda60753e

1.  アニメーション効果の右上チェックボックス横のテキストを「アニメーション効果」から「震える(アニメーション効果)」のように，選択されているスクリプト名を表示するようにできます．[[詳細](#アニメーション効果のスクリプト名を右上のチェックボックス横に表示する機能)]

    ![フィルタ名表示変更](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/251914ff-abf4-4854-945f-3fb15675d18c)

1.  標準描画の `X`, `Y`, `Z` などの「関連トラック」で `Shift` を押さなくても独立して変化方法を指定できるようになります．[[詳細](#関連トラックの変化方法指定時の-shift-キーを反転する機能)]

    ![関連トラックの例](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/9d09351a-a9ec-418a-9182-28885edd4bf1)

    - 代わりに `Shift` を押しているときは関連トラックも連動して変化方法が設定されるようになります．

1.  トラックバーの変化方法をツールチップの形で表示できます．[[詳細](#トラックバーの変化方法をツールチップで表示する機能)]

    ![ツールチップの表示](https://github.com/user-attachments/assets/f73ca4c4-f283-478d-a8fe-3068261f6b1e)

    パラメタや，加速・減速の設定も表示されます．

1.  トラックバーの変化方法のパラメタ設定を，ホイールクリックで表示させることができます．[[詳細](#トラックバーの変化方法のパラメタ設定をホイールクリックで表示する機能)]

    ![変化方法のパラメタ設定](https://github.com/user-attachments/assets/142fd32f-89ae-41a5-8787-b4f788878473)

    - トラックバーの名前ボタンをホイールクリック．


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

### トラックバーの数値入力ボックスのドラッグでマウス位置を固定する機能

トラックバーの数値入力ボックスをドラッグして数値を動かす際，マウスカーソルの位置が動かなくなります．

- 初期状態だと機能は無効化されています．有効にするには[設定ファイル](#trackmouse)で有効化してください．

- アイデア元: https://x.com/AviUtl_Support/status/1809973686382055680


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

### テキスト入力ボックスに文字を入力中はマウスカーソルを非表示にする機能

テキストオブジェクトやスクリプト制御のテキスト入力ボックス上にマウスカーソルがある状態で文字を入力すると，マウスカーソルが自動的に非表示になります．マウスを少し動かしたり，フォーカス移動が起こると再度表示されます．

- 他のテキスト入力ボックス（例えば，行間や字間の調整やトラックバー横の数値ボックス）では機能しません．

- なるべくなら Windows のマウス設定の「文字の入力中にポインターを非表示にする」を利用してください．Windows 側の設定が思い通りに動かない場合の代替策としてご利用ください．

### テキスト入力ボックスでの `TAB` 文字の幅を調整できる機能

テキストオブジェクトとスクリプト制御のテキスト入力ボックスの `TAB` 文字幅を，Windows 標準の `32` という値から変更できます．[設定ファイルを編集](#textboxtweaks)して変更してください．

- テキストオブジェクト，スクリプト制御それぞれで個別の値を設定できます．
- こちらも私の[エディットボックス最適化プラグイン改造版](https://github.com/sigma-axis/AviUtl-Plugin-OptimizeEditBox)で搭載していた機能ですが移植しました．

### テキスト入力ボックスでの `TAB` 文字入力を別の文字列に置き換える機能

テキストオブジェクトやスクリプト制御のテキスト入力ボックスで `TAB` 文字を入力すると，代わりに予め指定した別の文字列が入力されるようになります．

- キーボードによる通常の `TAB` 入力のみが対象です．例えばクリップボードからの貼り付けや，`Ctrl+TAB` による `TAB` 文字挿入では機能しません．
- テキストオブジェクトとスクリプト制御それぞれで個別に設定できます．
- 初期状態では無効化されています．利用する場合は[設定ファイルを編集](#textboxtweaks)して文字列を指定してください．

例えばテキストオブジェクトでの `TAB` 文字入力を全角空白 2 文字にしたり，制御文字で `<p+80,+0>` として 80 ピクセルの余白を挿入したりできます．スクリプト制御では半角空白 4 文字の入力に置き換えるなど，テキストオブジェクトと別の設定ができます．

### ドロップダウンリストから名前のキー入力で項目選択できる機能

テキストオブジェクトのフォント指定や，アニメーション効果のスクリプトを選ぶ際のドロップダウンリストで，選びたいものの名前がわかっている場合，キーボード入力で名前の先頭を入力するとその項目に飛べるようになります．

- 元来 Combo Box のコントロールが固有で持っている機能でしたが，AviUtl の特殊事情のためかきちんと働いていなかったのを再調整しました．

  いくつか制約があります:
  1. キーボード入力を受け付けるのはドロップダウンリストを開いている間のみです．フォーカスが当たっているだけの状態では機能しません．
  1. ドロップダウンリストを開いている間は AviUtl で設定したショートカットキーは機能しません．
  1. 半角英数の文字入力しか受け付けません（~~Windows 標準の仕様をそのまま引き継いでいるため~~ 追調査しましたが半角英数以外を受け付けない原因は不明）．

- [設定ファイル](#dropdownkeyboard)でこの機能の有効化・無効化を切り替えられます．

### アニメーション効果のスクリプト名を右上のチェックボックス横に表示する機能

元々は単に「アニメーション効果」と書かれていたテキストを「震える(アニメーション効果)」のように，選択されているスクリプト名も表示されるようになります．

- [設定ファイル](#filtername)でこの機能の有効化・無効化を切り替えたり，書式を指定したりできます．

#### 発案者

- [兎](https://twitter.com/rabb_et)様 / [AviUtl知識共有(Discord)](https://t.co/PRrPkEHI3w)

- https://discord.com/channels/1254055609578426499/1254056085619216436/1256590460617756683


#### 実装

- [蛇色](https://github.com/hebiiro)様

  https://github.com/hebiiro/aviutl.filter_name.auf

- このプラグインへの移植を提案してくださいました．
- ~~オリジナルと比べて書式が固定されている点で制約があります．~~
- **蛇色様のオリジナルを使う場合はこの機能は OFF にしておいてください．**

### 関連トラックの変化方法指定時の `Shift` キーを反転する機能

標準描画の `X`, `Y`, `Z` などは「関連トラック」となっていて，1つに対してトラックバー変化方法を選ぶと自動的に他の項目にも同じ変化方法が設定されるようになっています．`Shift` キーを押しながら選択することで独立して指定できますが，この挙動を反転して，

- `Shift` キーを押していないときは独立に指定
- `Shift` キーを押しているときは連動して指定

という挙動にします．

- 初期状態では OFF になっているので[設定ファイル](#easings)で有効化してください．

### トラックバーの変化方法をツールチップで表示する機能

トラックバーの名前のボタン部分にマウスカーソルを置くとツールチップが表示され，変化方法の名前や設定パラメタ，加速・減速の設定を，メニューを開かずに確認することができます．

- トラックバーの変化方法を「移動無し」にしている場合は表示されません．

- カーソルを置いてから表示されるまでの時間や，表示が自動的に消えるまでの時間などは[設定ファイル](#easings)で調整できます．

- 初期状態では OFF になっているので[設定ファイル](#easings)で有効化してください．

### トラックバーの変化方法のパラメタ設定をホイールクリックで表示する機能

トラックバーの名前ボタンをホイールクリックで，パラメタ設定ウィンドウを表示させることができます．

- 普通に左クリックしてから「設定」を選ぶ，または `Alt` + 左クリックも標準動作としてあります．それにホイールクリックも加えた形です．使いやすい入力方法を自由に選んでください．

- [設定ファイル](#easings)でこの機能の有効化・無効化を切り替えられます．


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

- `hide_cursor` で[文字入力中にマウスカーソルを非表示にする機能](#テキスト入力ボックスに文字を入力中はマウスカーソルを非表示にする機能)を有効化・無効化できます．

  `0` で無効，`1` で有効です．初期設定だと `hide_cursor=0` で無効．

- `tabstops_text` と `tabstops_script` で[入力ボックスの `TAB` 文字の幅](#テキスト入力ボックスでの-tab-文字の幅を調整できる機能)を指定できます．初期設定だと両方とも `-1` （Windows のデフォルト設定を利用）が指定されています．
  - `-1` を指定するとデフォルト値のままです（このプラグインによる介入もしません）．
  - Windows のデフォルト値は `32` です．

- `replace_tab_text` と `replace_tab_script` で [`TAB` 文字入力で指定の文字列を入力する機能](#テキスト入力ボックスでの-tab-文字入力を別の文字列に置き換える機能)の設定ができます．
  - 置き換えたい文字列を指定します．

    指定文字列はシングルクォート (`'`) やダブルクォート (`"`) で囲うのを推奨します．冒頭や末尾が空白文字（半角空白や `TAB` 文字を含む）の場合 `.ini` 形式の仕様で無視されるため必須です．

  - `TAB` 文字 1 字だけの文字列を指定することで，この機能を無効化できます．
  - エスケープ文字などの機能はないため，改行文字などの制御文字は一部（`TAB` や空白など）を除いて使用できません．
  - 設定ファイルは UTF-8 形式で保存しないと文字化けする可能性があります．
  - 文字数は UTF-8 で 256 バイトまでです．

  初期設定は以下の通りです:
  ```ini
  replace_tab_text="	"
  replace_tab_script="	"
  ```
  - テキストオブジェクトとスクリプト制御の両方で `TAB` 文字 1 字の指定で，機能が無効な状態です．

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

  初期設定は以下の通りです:
  ```ini
  updown=1
  keys_decimal=shift
  def_decimal=0
  keys_boost=alt
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

[設定ダイアログのトラックバーの数値をマウスホイール操作で動かせる機能](#トラックバー付近でマウスホイールで数値を操作)と[トラックバー横の数値入力ボックスのドラッグでマウス位置を固定する機能](#トラックバーの数値入力ボックスのドラッグでマウス位置を固定する機能)の設定ができます．

- `wheel` でホイール操作の機能を有効化・無効化できます．

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

- `fixed_drag` で数値部分のドラッグでのマウス位置固定機能を有効化・無効化できます．

  `0` で無効，`1` で有効です．初期設定だと `fixed_drag=0` で無効．

初期設定は以下の通りです:
```ini
wheel=1
keys_activate=ctrl
reverse_wheel=0
keys_decimal=shift
def_decimal=0
keys_boost=alt
rate_boost=10
cursor_react=1
fixed_drag=0
```
- `Ctrl` を押しながらマウスホイールで整数単位の移動．
- ホイール方向は既定値．
- `Shift` キーで小数点以下の最小単位で移動．
- `Alt` キーで 10 倍移動．`Shift+Alt` で小数点以下の最小単位の 10 倍．
- カーソル位置が対象範囲でかつ `Ctrl` キーが押されている状態だとカーソル形状が変化．
- 数値部分のマウス固定ドラッグ機能は無効．

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


初期設定は以下の通りです:
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

### `[FilterName]`

[アニメーション効果のスクリプト名を右上のチェックボックス横に表示する機能](#アニメーション効果のスクリプト名を右上のチェックボックス横に表示する機能)の有効，無効を切り替えたり，書式を指定できます．

`anim_eff_fmt` で チェックボックスに表示するテキストの書式を指定します．

- 指定文字列中の `{}` の部分がスクリプト名に置き換わります．

  - 開き波括弧 `{` を書式に含めたい場合，`{{` のように2つ並べます．

  例:

  | `anim_eff_fmt` | スクリプト名 | 表示 |
  |---|---|---|
  | `"{}(アニメーション効果)"` | `震える` | `震える(アニメーション効果)` |
  | `"アニメ効果の「{}」"` | `震える` | `アニメ効果の「震える」` |
  | `"{{{}}"` | `震える` | `{震える}` |
  | `"{{}{}{}{{}"` | `震える` | `{}震える震える{}` |

- `""` (空の文字列) を指定するとこの機能を無効化します．

- 初期値は `"{}(アニメーション効果)"` で，例えば `震える` だと `震える(アニメーション効果)` と表示されるようになります．

### `[Easings]`

トラックバーの変化方法編集に関する設定を変更できます．

- `linked_track_invert_shift` で[`Shift` キーを反転する機能](#関連トラックの変化方法指定時の-shift-キーを反転する機能)を有効化・無効化できます．

  `0` で無効，`1` で有効です．初期設定だと `linked_track_invert_shift=0` で無効．

- `wheel_click` で[パラメタ設定をホイールクリックで表示する機能](#トラックバーの変化方法のパラメタ設定をホイールクリックで表示する機能)を有効化・無効化できます．

  `0` で無効，`1` で有効です．初期設定だと `wheel_click=1` で有効．

- `tooltip` で[ツールチップ表示](#トラックバーの変化方法をツールチップで表示する機能)を有効化・無効化できます．

  `0` で無効，`1` で有効です．初期設定だと `tooltip=0` で無効．

- `tip_anim` でツールチップの表示のフェードインアニメーションを有効化・無効化できます．

  `0` で無効，`1` で有効です．初期設定だと `tip_anim=1` で有効．

- `tip_delay` で，マウスがボタン上に移動してからツールチップが表示されるまでの時間を指定できます．

  指定はミリ秒 ($\tfrac{1}{1000}$ 秒) 単位で，最小値は `0`, 最大値は `60000` (1 分), 初期値は `340` (0.34 秒) です．

- `tip_duration` で，マウスを動かしていなくてもツールチップが自動的に消えるまでの存続時間を指定できます．

  指定はミリ秒単位で，最小値は `0`, 最大値は `60000` (1 分), 初期値は `10000` (10 秒) です．

- `tip_text_color` でツールチップのテキスト色を指定できます．

  色は `0xRRGGBB` の形式で指定します．`-1` を指定するとシステムのデフォルトの色になります．初期値は `-1` でデフォルトの色．

  - ダークモード化プラグインのバージョンや設定などによってはテキストが見えにくい色になることがあったため，その対策のための設定項目です．
    - ダークモード化プラグインのバージョンや設定によっては反映されないことがあります．
    - なるべくならダークモード化プラグインのバージョン更新や設定変更での対応を試してみてください．
    - [アルティメットプラグイン](https://github.com/hebiiro/anti.aviutl.ultimate.plugin)の `r35` (記述時点での最新) だと，この設定は反映されませんが見やすいテキスト色で表示されているのを確認しています．

- 以上のツールチップに関する設定は，ツールチップ機能が無効 (`tooltip=0`) の場合無視されます．

初期設定は以下の通りです:
```ini
linked_track_invert_shift=0
wheel_click=1
tooltip=0
tip_anim=1
tip_delay=340
tip_duration=10000
tip_text_color=-1
```
- `Shift` を押しながら変化方法を選択はデフォルトの挙動 (`Shift` なしで関連トラックをまとめて変更，`Shift` ありで独立に変更).
- ホイールクリックによるパラメタ設定の表示は有効．
- ツールチップは無効．


## 謝辞

- [テキスト入力時のプレビュー画面一括更新の機能](#テキスト入力ボックスでのime入力したときの画面更新を一括にする機能)は，アイデア，実装方法を含めて hebiiro 様の[エディットボックス最適化プラグイン](https://github.com/hebiiro/AviUtl-Plugin-OptimizeEditBox)を参考にさせていただきました．このような場で恐縮ですが大変便利なプラグインの作成・公開に感謝申し上げます．

- [トラックバー横の :arrow_backward: :arrow_forward: をクリックしたときの増減量を修飾キーで調整する機能](#トラックバー横の-arrow_backward-arrow_forward-ボタンの増減量を調整)は，アイデア，実装方法を含めて nanypoco 様の [updown プラグイン](https://github.com/nanypoco/updown)を参考にさせていただきました．この場ようなで恐縮ですが大変便利なプラグインの作成・公開に感謝申し上げます．

- [アニメーション効果のスクリプト名を右上のチェックボックス横に表示する機能](#アニメーション効果のスクリプト名を右上のチェックボックス横に表示する機能)は，[兎](https://twitter.com/rabb_et)様が Discord の [AviUtl知識共有](https://t.co/PRrPkEHI3w)サーバーで提案し，[蛇色](https://github.com/hebiiro)様が[実装](https://github.com/hebiiro/aviutl.filter_name.auf)したものを，蛇色様の提案でこのプラグインに移植したものです．このお二方に対して深い感謝を申し上げます．


## 改版履歴

- **v1.60** (2024-07-14)

  - 選択中のトラック変化方法をツールチップで表示する機能の追加．

  - トラック変化方法のパラメタ設定ウィンドウをホイールクリックで開く機能の追加．

  - 関連トラック変化方法指定時の `Shift` キー反転機能の実装を簡易化．

- **v1.50** (2024-07-11)

  - 関連トラックの変化方法指定時に，`Shift` キーの認識を反転する機能を追加．

- **v1.42** (2024-07-10)

  - トラックバーの数値入力ボックスのドラッグ中にホイール操作をした場合は，マウス位置に関わらずドラッグ中の数値が動くように変更．

  - アニメーション効果のスクリプト名表示機能を刷新，書式指定ができるように．

    - v1.40 と v1.41 の設定ファイルの `[FilterName]` 以下の部分は互換性がなくなりました．機能を無効化していた場合，更新した `.ini` ファイルで `anim_eff_fmt=""` の設定をしてください．

- **v1.41** (2024-07-09)

  - トラックバーの数値入力ボックスのドラッグでマウス位置を固定する機能を有効にしている場合，ドラッグ中はカーソルを非表示にするように変更．

  - トラックバーの数値入力ボックスのドラッグ中に `Ctrl`, `Shift`, `Alt` を操作するとカーソル形状が意図しないものに変わることがあったのを修正．

- **v1.40** (2024-07-08)

  - トラックバーの数値入力ボックスのドラッグでマウス位置を固定する機能を追加．

- **v1.31** (2024-07-01)

  - アニメーション効果のスクリプト名表示機能が有効な状態でフィルタ名調整プラグインを検出した場合，メッセージを表示して Reactive Dialog 側の機能を無効化するように変更．

- **v1.30** (2024-07-01)

  - アニメーション効果のスクリプト名を右上のチェックボックス横に表示する機能を追加．

    - [兎](https://twitter.com/rabb_et)様の提案で蛇色(@hebiiro)様が[実装](https://github.com/hebiiro/aviutl.filter_name.auf)したものを，蛇色様の提案でこのプラグインに移植しました．この場を借りてお二方に感謝申し上げます．

- **v1.23** (2024-06-22)

  - 小数点以下が 3 桁以上のトラックバーの数値も正しく取り扱えるよう変更．

    標準だと影響ありませんが，拡張編集プラグインでは小数点以下 3 桁以上も実現できるのでその対処．

- **v1.22** (2024-04-01)

  - 半角英数を入力したときなど一部条件でマウスカーソルが非表示にされなかったことがあったのを修正．

- **v1.21** (2024-03-08)

  - 非表示にしたマウスカーソルの再表示手順の微調整．

- **v1.20** (2024-03-08)

  - テキストボックス上での `TAB` 文字入力を，予め指定した別文字列に置き換える機能追加．(Thanks to @CalumiQul for the suggestion!)

  - テキストボックス上で編集中にマウスカーソルを非表示にする機能追加．

- **v1.10** (2024-02-29)

  - フォント選択などのドロップダウンリストから名前のキーボード入力で選べる機能を追加．

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


##  AviUtl『フィルタ名調整』プラグイン

https://github.com/hebiiro/aviutl.filter_name.auf

---

MIT License

Copyright (c) 2024 hebiiro

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
