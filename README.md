# ジュイスティックモジュールを使ったマウス JoyPoint Firmware

![Alt text](docs/photo1.jpg)

## 特徴

- I2C 接続、Pimoroni Trackball モジュール PIM447 互換
- Grove の I2C ソケットをもち、M5Stack などを接続可能

## ドキュメント

- [JoyPoint v1.0.1 回路図](docs/joypoint-v1.0.1-circuit.pdf)

## JoyPoint v1.0.1 ファームウェア更新方法

ATTiny402 を使用するため、UPDI プログラマが必要です。

UPDI プログラマは、USB シリアル変換を使って制作可能です。こちらを確認ください。

https://github.com/microchip-pic-avr-tools/pymcuprog#serial-port-updi-pyupdi

PlatformIO を使います。VS Code に拡張機能 PlatformIO をインストールしてください。
VS Code で joypoint ディレクトリをワークスペースとして開きます（コマンド「ファイル：フォルダーを開く（File: Open Folder...）」で joypoint ディレクトリを開きます）

platform.ini の upload_port、upload_command などを調整し、アップロードをしてください。

※ 後日、詳細手順を記載します
