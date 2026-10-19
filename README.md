# LiNEA40 ZMK config

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

## 参考

- [ZMK Bluetooth名変更処理](https://github.com/zmkfirmware/zmk/blob/v0.2.1/app/src/ble.c)
- [Zephyrの名前保存処理](https://github.com/zmkfirmware/zephyr/blob/v3.5.0%2Bzmk-fixes/subsys/bluetooth/host/hci_core.c)
