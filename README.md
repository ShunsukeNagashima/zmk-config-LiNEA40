# LiNEA40 ZMK config

## BluetoothプロファイルごとのMac／Windows配列

`config/LiNEA40.keymap` では、プロファイル **1をWindows用**、**0・2・3・4をMac用**にしています。
Escapeを長押しして無線設定レイヤーに入り、`U` の位置でプロファイル1を選びます。
Macへ戻るときは `Y`（0）、`I`（2）、`O`（3）、`P`（4）の位置で接続先を選びます。

| キー位置 | Mac用プロファイル | Windows用プロファイル1 |
| --- | --- | --- |
| 左下のCommand | Command | Ctrl |
| 左下のCtrl | Ctrl | Windowsキー |
| 左下のOption | Option | Alt |
| `A`＋左Shiftの同時押し | Command＋Space | Alt＋バッククォート（日本語入力オン／オフ） |
| 中央クリック（`Y`＋`U`の同時押し） | マウス中央クリック | Win＋Tab（タスクビュー） |

通常・記号・数字レイヤーの対象キーに適用します。
文字、数字、中央クリック以外のマウス操作、レイヤー番号、ファンクション／無線設定レイヤー固有のキーは従来どおりです。
Windows側は、日本語IMEのハードウェアキーボードレイアウトを英語（101/102キー、US配列）に設定してください。
IMEはWindows標準のMicrosoft IMEを想定しています。
日本語のMicrosoft IMEを選択した状態で、`A`＋左Shiftにより「あ」と「A」を切り替えます。
入力言語そのものを切り替える操作ではありません。
Windowsの入力切り替えだけは、バッククォートを離してからAltを離す順序で送信します。
`os_input` の `windows-separate-mod-release` で指定しており、Mac用プロファイルの
Command＋Spaceや他のキーには適用しません。全OS共通の修飾キー分離設定は使用しません。
別のIMEやカスタムキー設定で動作しない場合は、そのIMEのオン／オフ操作に合わせて
`dts/behaviors/linea40_profile_keys.dtsi` の `os_input` の `windows-keycode` を変更します。

対象キーを押すたびに、ZMKが保持する選択中のBluetoothプロファイルを参照します。
再起動後も復元されたプロファイルに従い、別途OSモードを切り替える必要はありません。
USB出力時も選択中のBluetoothプロファイルに従います。
キーを押したままプロファイルを変更した場合は、押下時のキーコードでキーを解除します。
Bluetooth名のHome／Work切り替えとは独立しており、ペアリングの消去は不要です。

### Studioで保存済みのキーマップを使う場合

保存済みキーマップがある場合、ファームウェア更新だけではキーの動作が置き換わりません。
Studioで、通常・記号・数字レイヤーの該当キーを以下に置き換えて保存してください。
透過キーはそのままで構いません。

- Commandの位置：`Profile Key: Command / Ctrl`
- Ctrlの位置：`Profile Key: Ctrl / Windows`

`A`＋左Shiftの同時押しはファームウェア側のコンボ設定で切り替わるため、Studioでの割り当て変更は不要です。
入力切り替えを別の単独キーに置く場合は `Profile Key: Input Switch` を使えます。
中央クリックの同時押しもファームウェア側で切り替わるため、Studioでの変更は不要です。
別の単独キーに置く場合は `Profile: Middle Click / Task View` を使えます。

同じ動作を複数レイヤーの同じキー位置に使えます。
各動作は同時に1か所の押下を扱うため、複数の物理キーに割り当てて同時押しする用途には使わないでください。

### Windows用プロファイル番号の変更

`dts/behaviors/linea40_profile_keys.dtsi` の `LINEA40_WINDOWS_PROFILE` を変更して再ビルドします。
対象の動作がこの設定を共有します。

### Keymap EditorでMac／Windowsを見比べる

GitHub連携のKeymap Editorで、ブランチを `main`、キーマップを **`LiNEA40-OS-preview`** に切り替えます。
一覧に出ない場合はリポジトリを再読み込みしてください。

- 通常配列：`Mac_default_layer`（0）と `Windows_default_layer`（5）を比較
- 記号配列：`Mac_MARK`（1）と `Windows_MARK`（6）を比較
- 数字配列：`Mac_Number`（2）と `Windows_Number`（7）を比較
- 入力切り替え：Combosの `Mac_change_source` と `Windows_change_source` を比較

独自キーを通常の `&kp` に展開しているため、CtrlやGUIキー、入力切り替えのショートカットを直接確認できます。
エディターの歯車設定 → **Platform specific glyphs** を **Windows** にすると、GUIキーがWindowsマークになります。
Macの表示には **macOS** を選びます。この表示設定は全レイヤー共通で、ファームウェアの動作には影響しません。

このキーマップは**比較専用の自動生成データ**です。ここを編集しても本体用ファームウェアには反映されません。
実際の変更は `LiNEA40.keymap` または `dts/behaviors/linea40_profile_keys.dtsi` に行います。

**Keymap Editorから `main` に保存・コミットすると、比較用表示も自動更新されます。**
GitHub Actionsの **Update OS preview** が比較用の2ファイルを再生成し、差分がある場合だけ自動コミットします。
処理が完了したらKeymap Editorを再読み込みし、`LiNEA40-OS-preview` を選んでください。
画面で編集中の段階では更新されず、GitHubへのコミット後に反映されます。

自動更新は `main` が対象です。ローカルや別ブランチで確認したい場合は `make preview` を使えます。
処理中に次の変更が保存された場合は、最新の設定から再生成して反映します。
自動コミットが次のワークフローを起動することはなく、本体用ファームウェアは元の編集コミットでビルドされます。
自動更新に失敗した場合はActionsの実行結果を確認し、**Update OS preview → Run workflow** から再実行できます。

`make test` では比較データが最新の設定と一致することも確認します。
ビルド対象は引き続き `LiNEA40` で、比較用レイヤーは本体には追加されません。

## Bluetooth名を本体ごとに選ぶ

同じ左右のファームウェアを2台のLiNEA40に書き込み、各キーボードで名前を選択できます。
名前はMacに接続する親機（右側）に保存されます。

| 操作 | 保存する名前 |
| --- | --- |
| 無線設定レイヤーで `N` の位置を押す | `LiNEA40-Home` |
| 無線設定レイヤーで `M` の位置を押す | `LiNEA40-Work` |

現在の `config/LiNEA40.keymap` では、`Escape` を長押しすると無線設定レイヤーに入ります。
キーを離しても、再起動しても、選んだ名前は維持されます。
通常のファームウェア更新でも維持され、名前変更によるペアリング情報の消去は行いません。
`settings_reset` で保存設定を消去すると名前も初期値の `LiNEA40` に戻ります。

### 初回の書き込み

1. GitHub Actionsの `firmware` をダウンロードし、左右それぞれに対応する通常ファームウェアを書き込みます。
2. 1台目でHome、2台目でWorkを選びます。名前変更のための `settings_reset` は不要です。
3. Macに以前の名前が残る場合は、Bluetooth設定で対象キーボードの登録を削除し、再ペアリングします。
   キーボード側も対応するBluetoothプロファイルを `BT_CLR` でクリアしてください。
   2台を1台ずつ電源オンにすると対象を区別できます。

### ZMK Studioでキーマップを保存済みの場合

保存済みのキーマップが優先されるため、書き込みだけでは新しいキー配置が反映されない場合があります。
USB接続でZMK Studioを開き、以下の動作を好きなキーに割り当てて保存してください。

- `Bluetooth Name: Home`
- `Bluetooth Name: Work`

既存のキーマップを維持したまま追加できます。
この機能が変更するのはBluetooth名であり、USB製品名やStudioの機器名は変更しません。

### 名前の候補を変更する

`dts/behaviors/linea40_bluetooth_names.dtsi` の `device-name` を編集して再ビルドします。
名前は英数字・記号で16文字以内にしてください。
更新後、各本体で対応するキーをもう一度押すと新しい候補名を保存します。

## ビルド

GitHub Actionsでは `zephyr/module.yml` により、このリポジトリの独自動作を自動で読み込みます。
Dockerを使う既存のローカルビルドは `make build container_name=<コンテナ名>` で実行します。
手動でビルドする場合は、既存の `ZMK_EXTRA_MODULES` にこのリポジトリの絶対パスを追加してください。

## テスト

`make test` で、プロファイル別キー処理のホストテストを実行します（Python 3とCコンパイラが必要）。
実際のドライバをZMK APIの代替実装と組み合わせ、全プロファイル、切り替え中のキー解除、
修飾キーの同時押し、ショートカット、Studioメタデータ有無を検証します。
ファームウェア全体のビルド・実機でのBluetooth／USB入力確認は別途必要です。

## 参考

- [ZMK Bluetooth名変更処理](https://github.com/zmkfirmware/zmk/blob/v0.2.1/app/src/ble.c)
- [Zephyrの名前保存処理](https://github.com/zmkfirmware/zephyr/blob/v3.5.0%2Bzmk-fixes/subsys/bluetooth/host/hci_core.c)
- [Microsoft IMEのショートカット](https://support.microsoft.com/en-us/windows/hardware/input-devices/microsoft-japanese-ime)
