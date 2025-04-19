# Reactive Dialog AviUtl プラグイン

拡張編集の設定ダイアログへのデータ入力・調整・操作方法を拡充するプラグインです．

[ダウンロードはこちら．](https://github.com/sigma-axis/aviutl_reactive_dlg/releases) [紹介動画1．](https://www.nicovideo.jp/watch/sm43602590) [紹介動画2．](https://www.nicovideo.jp/watch/sm44864400)


## 機能紹介

以下の機能があります．各機能は[設定ファイル](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/設定ファイルについて)で個別に有効化 / 無効化できます．

詳しい使い方は [Wiki](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki) を参照してください．

1.  テキスト入力ボックスから `TAB` キーでフォーカスを移動する機能．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/テキスト入力ボックス関連の機能#テキスト入力ボックスからフォーカス移動)]

    ![テキストボックス間でフォーカス移動](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/6f99656b-ce7d-4b2c-820d-f097a3906a5d)

    - 設定で `TAB` キー以外のキーに割り当てることもできます．
    - 相補的に，ショートカットキーで[テキスト入力ボックスにフォーカスを移動するコマンドも追加](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/テキスト入力ボックス関連の機能#テキスト入力ボックスにフォーカス移動)します．

1.  トラックバーの数値入力ボックスにもショートカットキーでフォーカスを移動するコマンドを追加．[[詳細](
  https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーのキーボード入力関連の機能#トラックバーの数値入力ボックスにフォーカス移動)]

1.  トラックバーの数値入力ボックス上での :arrow_up: / :arrow_down: キー入力で数値を操作できる機能．[[詳細](
  https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーのキーボード入力関連の機能#トラックバーのキーボード入力関連の機能#トラックバーの数値入力ボックスで-arrow_up--arrow_down-キーを押して数値変更)]

    ![上下キーで数値操作](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/a56fc200-31e7-4f77-96bd-c899edd87c1c)

    - `Ctrl` / `Shift` / `Alt` キーと組み合わせて小数点以下の最小単位や10倍移動などもできます．
    - `ESC` キーで編集前の値に戻します．既に編集前の値ならフォーカスを外します．[[詳細](
  https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーのキーボード入力関連の機能#数値入力ボックスで-esc-キーを押して元に戻す)]
    - `100` :left_right_arrow: `-100` などの符号反転をショートカットキーで手早く行うこともできます．[[詳細](
  https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーのキーボード入力関連の機能#トラックバーの数値入力ボックスで指定キーで数値の符号を反転)]

1.  トラックバーの数値入力ボックス上でのショートカットキー入力で，トラックバーの変化方法選択メニューを表示させる機能．[[詳細](
  https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーのキーボード入力関連の機能#トラックバーの変化方法をショートカットキーから変更)]

    ![変化方法のメニューをショートカットキーで表示](https://github.com/user-attachments/assets/6be40de8-6bf3-4bb1-bbad-7a339cc2fdcd)

1.  トラックバー付近での `Ctrl` + マウスホイールで数値を操作できる機能．[[詳細](
  https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーのマウス入力関連の機能#トラックバー付近でマウスホイールで数値を操作)]

    ![Ctrl+マウスホイールで数値操作](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/b3600538-b0ba-4e4e-bbff-b6bb0957a181)

    - `Ctrl` / `Shift` / `Alt` キーと組み合わせて小数点以下の最小単位や 10 倍移動などもできます．
    - 同時押しに使う `Ctrl` キーも含めて，設定でキー割り当ての変更ができます．

1.  トラックバーの数値入力ボックスのドラッグでマウス位置を固定する機能．[[詳細](
  https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーのマウス入力関連の機能#ドラッグでマウス位置を固定する機能)]

    ![ドラッグしてもマウス位置固定](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/6bb074db-0a38-49e6-ba9f-095f131272e2)

    - このデモから更にバージョンが上がってカーソルが非表示になりました．

1.  トラックバーの数値入力ボックスのドラッグ動作をカスタマイズする機能．[[詳細](
  https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーのマウス入力関連の機#ドラッグでの移動量を修飾キーや右クリックで調整)]

    https://github.com/user-attachments/assets/7162a4b3-fb93-4d4e-81de-b66e1d3e95ce

    - `Alt` を押していると増減量が 10 倍に．
    - マウス右ボタンを押している間は「10 ピクセル動くごとに1段階」と階段状の操作ができます．
    - ドラッグ方向を縦方向や逆方向に変更できます．

1.  トラックバー横の :arrow_backward: / :arrow_forward: ボタンを押したときの移動量を修飾キーで調整できる機能．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーのボタン操作関連の機能#トラックバー横の-arrow_backward-arrow_forward-ボタンでの増減量を調整)]

    ![ボタン操作に修飾キーを使えるように](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/e360a09c-aa10-4250-a567-783605235172)

    - `Ctrl` / `Shift` / `Alt` キーと組み合わせて小数点以下の最小単位や10倍移動などもできます．
    - nanypoco 様の [updown プラグイン](https://github.com/nanypoco/updown)とほぼ同等の機能です．実装手法を含め参考にさせていただきました．若干カスタマイズの自由度が異なります．

1.  テキスト入力ボックスにIME入力などで複数文字を同時に入力した場合，1文字ずつ追加されてその度に画面が更新されるのを，一括で画面更新する機能．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/テキスト入力ボックス関連の機能#ime入力したときの画面更新を一括にする機能)]

    https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/78c14923-c361-4da7-8721-5b6e8c19d708

    - hebiiro 様の[エディットボックス最適化プラグイン](https://github.com/hebiiro/AviUtl-Plugin-OptimizeEditBox)で実装されていたものと類似の機能です（現在は機能削除されています）．不具合修正や実装方式を見直すなどして搭載しました．

1.  テキスト入力ボックスでの `TAB` 文字の幅を調整できる機能．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/テキスト入力ボックス関連の機能#tab-文字の幅を調整できる機能)]

    ![TAB文字幅の変更](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/09c8ca94-b099-465a-872e-1ee7cd0dfa60)

1.  テキスト入力ボックスで `TAB` 文字を入力した際，代わりに予め指定した別の文字列を入力する機能．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/テキスト入力ボックス関連の機能#テキスト入力ボックスでの-tab-文字入力を別の文字列に置き換える機能)]

    テキストオブジェクト内での `TAB` 文字はあってないような扱いなので，例えば半角空白 4 個だったり制御文字 `<p+80,+0>`（80 ピクセルのカーニング）など有意義な文字列に置き換えられます．

1.  フォント選択などのドロップダウンリストから名前をキー入力して項目を選べる機能．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/ドロップダウンリスト関連の機能#ドロップダウンリストから名前のキー入力で項目選択できる機能)]

    https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/27d72205-ed26-4df3-b98c-37eeda60753e

1.  アニメーション効果などスクリプトを利用するフィルタ効果で，右上チェックボックス横のテキストを「アニメーション効果」から「震える(アニメーション効果)」のように，選択されているスクリプト名を表示するようにできます．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/フィルタ効果関連の機能#選択中のスクリプト名を右上のチェックボックス横に表示する機能)]

    ![フィルタ名表示変更](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/251914ff-abf4-4854-945f-3fb15675d18c)

1.  フィルタ効果の右クリックメニューにコマンドを追加．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/フィルタ効果関連の機能#フィルタ効果の右クリックメニューに項目を追加)]

    ![フィルタ効果の追加メニュー](https://github.com/user-attachments/assets/666463fb-3913-4d7d-b9e4-b038556e1df3)

    `Luaスクリプトとしてコピー` を実行すると，スクリプト制御で使えるフィルタ効果と等価なテンプレートコードをクリップボードにコピーします．

    上の例だと次のテキストがコピーされます:
    ```lua
    obj.effect("シャドー","X",-40,"Y",24,"濃さ",40.0,"拡散",10,"影を別オブジェクトで描画",0,"color",0x290a66,"file","")
    ```

1.  折りたたんだフィルタ効果に対して，ツールチップで内容を簡易的に確認できます．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/フィルタ効果関連の機能#折りたたんだフィルタ効果の内容をツールチップで表示する機能)]

    ![折りたたんだフィルタ効果の内容](https://github.com/user-attachments/assets/80280756-14b5-4889-a45d-5ad334d99b3b)

1.  トラックバーの時間変化に関する情報をツールチップの形で表示できます．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーの変化方法関連の機能#トラックバーの名前のボタンにツールチップを表示)]

    ![ボタンの上にカーソルを合わせると表示](https://github.com/user-attachments/assets/ab6dd08f-e5b6-4968-8d1b-72663839fc65)

    1.  現在選択中の変化方法を表示．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーの変化方法関連の機能#変化方法に関する情報)]

        ![ツールチップの変化方法表示](https://github.com/user-attachments/assets/f73ca4c4-f283-478d-a8fe-3068261f6b1e)

        - パラメタや，加速・減速の設定も表示されます．

    1.  現在フレームでの計算値の表示．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーの変化方法関連の機能#現在の値の表示)]

        ![現在値表示](https://github.com/user-attachments/assets/ec0d9b4d-bd7f-43d0-b4d9-42f5f29c60ff)

    1.  前後の中間点の数値を表示．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーの変化方法関連の機能#前後の中間点での設定値)]

        ![ツールチップの数値表示](https://github.com/user-attachments/assets/b0b10a7b-ffc4-473d-ac25-503a335b9ec1)

        - 現在選択区間の数値は，`[ ... ]` で囲まれて表示されます．

    1.  選択区間内での数値の時間変化をグラフで表示．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーの変化方法関連の機能#時間変化のグラフ)]

        ![ツールチップのグラフ表示](https://github.com/user-attachments/assets/842726f2-faf9-4501-b1f2-64c6f292b47d)

    これらは個別に表示・非表示を設定できます．

1.  標準描画の `X`, `Y`, `Z` などの「関連トラック」で `Shift` を押さなくても独立して変化方法を指定できるようになります．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーの変化方法関連の機能#関連トラックの変化方法指定時の-shift-キーを反転)]

    ![関連トラックの例](https://github.com/sigma-axis/aviutl_reactive_dlg/assets/132639613/9d09351a-a9ec-418a-9182-28885edd4bf1)

    - 代わりに `Shift` を押しているときは関連トラックも連動して変化方法が設定されるようになります．

1.  トラックバーの変化方法のパラメタ設定を，ホイールクリックで表示させることができます．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーの変化方法関連の機能#トラックバーの変化方法のパラメタ設定をホイールクリックで表示)]

    ![変化方法のパラメタ設定](https://github.com/user-attachments/assets/142fd32f-89ae-41a5-8787-b4f788878473)

    - トラックバーの名前ボタンをホイールクリック．

1.  トラックバーの名前ボタンを右クリックで，数値をコピペしたり一括操作したりができるメニューが表示されるようになります．[[詳細](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーボタンの右クリックメニューの機能)]

    ![追加の右クリックメニュー](https://github.com/user-attachments/assets/ab46af13-2aff-4107-ac12-c7cd48713abe)

    - 中間点を含めた数値を一括でコピー & 貼り付けができます．
    - 中間点1つ分数値を前後にずらしたり，前後反転などの操作ができます．


## 動作要件

- AviUtl 1.10 + 拡張編集 0.92

  https://spring-fragrance.mints.ne.jp/aviutl
  - 拡張編集 0.93rc1 等の他バージョンでは動作しません．

- Visual C++ 再頒布可能パッケージ（\[2015/2017/2019/2022\] の x86 対応版が必要）

  https://learn.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist

## 導入方法

以下のフォルダのいずれかに `reactive_dlg.auf` と `reactive_dlg.ini` をコピーしてください．

1. `aviutl.exe` のあるフォルダ
1. (1) のフォルダにある `plugins` フォルダ
1. (2) のフォルダにある任意のフォルダ

## 謝辞

- [テキスト入力時のプレビュー画面一括更新の機能](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/テキスト入力ボックス関連の機能#ime入力したときの画面更新を一括にする機能)は，アイデア，実装方法を含めて hebiiro 様の[エディットボックス最適化プラグイン](https://github.com/hebiiro/AviUtl-Plugin-OptimizeEditBox)を参考にさせていただきました．このような場で恐縮ですが大変便利なプラグインの作成・公開に感謝申し上げます．

- [トラックバー横の :arrow_backward: :arrow_forward: をクリックしたときの増減量を修飾キーで調整する機能](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーのボタン操作関連の機能#トラックバー横の-arrow_backward-arrow_forward-ボタンでの増減量を調整)は，アイデア，実装方法を含めて nanypoco 様の [updown プラグイン](https://github.com/nanypoco/updown)を参考にさせていただきました．このような場で恐縮ですが大変便利なプラグインの作成・公開に感謝申し上げます．

- [アニメーション効果のスクリプト名を右上のチェックボックス横に表示する機能](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/フィルタ効果関連の機能#選択中のスクリプト名を右上のチェックボックス横に表示する機能)は，[兎](https://twitter.com/rabb_et)様が Discord の [AviUtl知識共有](https://t.co/PRrPkEHI3w)サーバーで提案し，[蛇色](https://github.com/hebiiro)様が[実装](https://github.com/hebiiro/aviutl.filter_name.auf)したものを，蛇色様の提案でこのプラグインに移植したものです．このお二方に対して深い感謝を申し上げます．

- トラックバーの名前ボタンを右クリックから使用できる機能の 1 つ[現在選択区間の一方の値を，もう一方の値で上書きするコマンド](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/トラックバーボタンの右クリックメニューの機能#上書き-左--右-30-上書き-左--右-40)のアイデアは，Ucchi98 様の [ParamCopy プラグイン](https://github.com/Ucchi98/AviUtlPlugins)が元になっています．素晴らしい機能の提案・利便性の実証に感謝申し上げます．

- [フィルタ効果の右クリックメニューから `obj.effect()` の Lua コードをコピーする機能](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/フィルタ効果関連の機能#luaスクリプトとしてコピー)は，nazonoSAUNA 様による Discord 上の抹茶鯖での提案を実装したものです．素晴らしい機能の提案に感謝申し上げます．


## 改版履歴

- **v2.10** (2025-04-??)

  - フィルタ効果の右クリックメニューに `Luaスクリプトとしてコピー` コマンドを追加，スクリプト制御で使えるフィルタ効果と等価なテンプレートコードをクリップボードにコピーします．

  - 折りたたんだフィルタ効果に対して，左上の折りたたみボタンや右上のチェックボックスにツールチップを表示する機能を追加．折りたたんだフィルタ効果の内容を簡易的にツールチップで確認できます．

  - トラックバーの名前ボタンのツールチップに表示項目を追加，現在選択フレームでの計算値が確認できるように．

  - ツールチップやクリップボードに表示/出力する中間点の区切り文字を，デフォルトの右矢印 `→` から設定で自由に変更できるように．

  - 設定ファイルに項目を追加，一部は移動/廃止されました．更新の際は再設定をお願いします．[[参考](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki/設定ファイルについて#設定ファイルの遍歴)]

- **v2.00** (2025-04-11)

  - 大幅コード整理．
    - **これに伴って設定ファイルの互換性が失われました．v1.90 以前から更新する際は，`reactive_dlg.ini` の更新と再設定をお願いします．**

  - トラックバーの数値入力ボックスをドラッグした際の動作変更を追加．
    - 各種修飾キーやマウス右ボタンで移動量を調整できるように．
    - 縦方向や逆方向のドラッグができるように．

  - トラックバーの値のコピー / 貼り付けや数値を一括操作する機能を，複数オブジェクト選択時にも動作するよう拡張．

  - トラックバーのツールチップに現在選択区間の時間変化をグラフで表示する機能を追加．

  - テキストボックス入力中にマウスカーソルを隠す機能を削除．

  - 機能説明書の主体を [GitHub の Wiki](https://github.com/sigma-axis/aviutl_reactive_dlg/wiki) に移行．

- **v1.90** (2025-02-19)

  - トラックバーのツールチップに前後の中間点の値を表示する機能を追加．
    - これに伴って，設定ファイルの `[Easings]` 以下の項目 `tooltip` は，`tooltip_mode` で変化方法の表示，`tooltip_values_left` と `tooltip_values_right` で数値の表示と機能が分けられました．更新の際には設定ファイルの修正も必要です．

  - トラックバーの値をクリップボードにコピー / 貼り付けする機能や，設定値を前後反転する機能などを追加．
    - トラックバーの名前のボタンを右クリックで表示されるメニューから使用できます．

- **v1.82** (2024-09-29)

  - トラックバー変化方法のツールチップ表示で無駄処理省略．

  - コード整理．

- **v1.81** (2024-09-27)

  - 符号を反転する機能が，一部他の機能を無効化していると動かなかったことがあったのを修正．

- **v1.80** (2024-09-26)

  - トラックバーの数値入力ボックスで，ショートカットキーで符号を反転する機能を追加．
    - 初期状態では無効化されています．利用する場合は設定ファイルで有効化してください．

  - トラックバーの数値入力ボックスで :arrow_up: / :arrow_down: での数値操作などを行った際に，テキストの範囲選択を解除しないよう変更．

- **v1.71** (2024-09-03)

  - [フィルタ名調整プラグイン](https://github.com/hebiiro/aviutl.filter_name.auf) (`filter_name.auf`) との競合メッセージと競合時の対処を修正．

- **v1.70** (2024-08-30)

  - トラックバーの変化方法選択のメニューをショートカットキーで表示する機能を追加．

  - アニメーション効果などのスクリプト名表示で，`.ini` ファイルに書式の指定がなかった場合文字化けしていたのを修正．

  - 一部処理を `.eef` による出力フィルタがあった場合に対応．

- **v1.62** (2024-07-15)

  - アニメーション効果のスクリプト名表示機能を，カスタムオブジェクト，カメラ効果，シーンチェンジにまで拡張．

- **v1.61** (2024-07-15)

  - 競合確認のメッセージ内で，設定ファイルの記述が間違っていたのを修正．

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

Copyright (C) 2024-2025 sigma-axis

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


##  AviUtl 用プラグイン - ParamCopy

https://github.com/Ucchi98/AviUtlPlugins

---

MIT License

Copyright (c) 2024 Ucchi

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
- Twitter: https://x.com/sigma_axis
- nicovideo: https://www.nicovideo.jp/user/51492481
- Misskey.io: https://misskey.io/@sigma_axis
- Bluesky: https://bsky.app/profile/sigma-axis.bsky.social
