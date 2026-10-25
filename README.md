# nesdumper

Arduino Mega 2560 と ファミリーコンピュータ（FC 60ピン）カセットコネクタを直結してROM（PRG-ROM / CHR-ROM）を吸い出し、エミュレータ用 `.nes` (iNES形式) ファイルを生成するツールです。

- **対応マッパー**：
  - **Mapper 0 (NROM)**：『忍者くん 魔城の冒険』等（16KB/32KB PRG + 8KB CHR）
  - **Mapper 3 (CNROM)**：『スターソルジャー』等（32KB PRG + 最大32KB CHR バンク切り替え）
  - **Mapper 4 (MMC3)**：『スーパーマリオブラザーズ3』等（256KB PRG + 128KB CHR 大容量バンク切り替え）
  - **Mapper 206 (Namcot 108/118)**：『プロ野球ファミリースタジアム'88』等（128KB PRG + 64KB CHR バンク切り替え）
  - **Mapper 185 (CNROM+SECURITY)**：『マイティボンジャック』等（32KB PRG + 8KB CHR プロテクト解除付き）
- `GOROman/nes-rom-reader` (`famidump v0.5 rev2`) のシリアル通信プロトコルとも完全互換性を持っています。

---

## 🎮 動作確認済みタイトル一覧（NesCartDB照合済み）

本ツールおよび配線で1ビットの誤りもなく100%完全吸い出し・エミュレータ起動を確認済みのタイトルです。

| タイトル | マッパー | PRG容量 / CRC32 | CHR容量 / CRC32 | 結合CRC32 | 判定 |
|---|:---:|---|---|:---:|:---:|
| **忍者くん 魔城の冒険** (ジャレコ) | 0 (NROM-128) | 16 KB / `7A3FCC31` | 8 KB / `22EA1607` | `A222F5A0` | ✅ 100%一致 |
| **スーパーマリオブラザーズ** (任天堂) | 0 (NROM-256) | 32 KB / `5CF548D3` | 8 KB / `867B51AD` | `D445F698` | ✅ 100%一致 (垂直ミラー) |
| **スターソルジャー** (ハドソン) | 3 (CNROM) | 32 KB / `E6C4EC06` | 32 KB (4バンク) / `67651E57` | `1B421E9C` | ✅ 100%一致 |
| **マイティボンジャック** (テクモ) | 185 / 0 (高互換) | 32 KB / `AA755715` | 8 KB / `5425A4D0` | `9C521240` | ✅ 100%一致 |
| **スーパーマリオブラザーズ3** (任天堂) | 4 (MMC3) | 256 KB (32バンク) / `64AC5F5E` | 128 KB (16バンク) / `A0AE2B4B` | `01FCF803` | ✅ 100%一致・動作確認済 |
| **プロ野球ファミリースタジアム'88** (ナムコ) | 4 / 206 (Namcot 118) | 128 KB (16バンク) / `F64590AC` | 64 KB (8バンク) / `C8F79AFE` | `B108EDC6` | ✅ 実機・エミュレータ動作確認済 |
| **4人打ち麻雀** (任天堂 / ハドソン) | 0 (NROM-128) | 16 KB / `3ED8C6B5` (PRG1) / `485EC98E` (PRG0) | 8 KB / `425CC61F` | `24KB` | ✅ 100%一致 (垂直ミラー) |

> [!IMPORTANT]
> **AIエージェント / 次回セッション再開時の必読ドキュメント**
> ハードウェア立ち上げ時に詰まったポイント・原因・解決策の時系列記録（開発日記）や、コネクタ刻印向きの仕様、環境固有情報はすべて **[docs/DEVLOG.md](docs/DEVLOG.md)** にまとめられています。セッション再開時は必ず最初に `docs/DEVLOG.md` を参照してください。

---

## 🛒 必要部品・動作確認済みハードウェア構成

本プロジェクトの実機検証において、実際に動作確認が取れている購入部品一覧です。はんだ付け不要で、コネクタ端子にジャンパーワイヤを直挿しして構築可能です。

| 部品名 | 型番 / 仕様 | 購入先リンク | 備考 |
|---|---|---|---|
| **マイコンボード** | ELEGOO MEGA2560 R3ボード (ATmega2560 + ATmega16U2) | [Amazon](https://www.amazon.co.jp/dp/B06ZZ259PM?ref=ppx_yo2ov_dt_b_fed_asin_title) | USBケーブル付属。Arduino Mega 2560 互換ボード |
| **ジャンパーワイヤ** | ELEGOO オスメスジャンパーワイヤ 200mm (50本入り) | [Amazon](https://www.amazon.co.jp/dp/B06ZZXH4XT?ref=ppx_yo2ov_dt_b_fed_asin_title) | 60Pコネクタ（オス側をスリットに挿入）⇔ Arduino（メス側をピンヘッダに挿入）接続用（約40本使用） |
| **60Pカードエッジコネクタ** | ヒロセ電機 (HRS) `CR22A-60D-2.54DS(70)` | [千石電商](https://www.sengoku.co.jp/mod/sgk_cart/detail.php?code=EEHD-5BYP) | 2.54mmピッチ、60極（30極×2列）ローコスト高性能カードエッジコネクタ。※コネクタ刻印番号の向きは後述の配線表を参照 |

---

## 📁 プロジェクト構成（専用ツールの置き場所）

| パス | 役割・説明 |
|---|---|
| [`firmware/nesdumper/nesdumper.ino`](firmware/nesdumper/nesdumper.ino) | **Arduino Mega 2560 用ファームウェア本体**（115200 bps、CRC32計算・`famidump`互換コマンド・診断用 `SCAN`/`TEST` 搭載） |
| [`web/index.html`](web/index.html) | **ブラウザ吸い出しツール（推奨）**。インストール不要で Chrome / Edge で開くだけで WebSerial 経由のROM吸い出し・CRC32検証・CHRドット絵プレビュー・`.nes` 保存が可能 |
| [`host/nesdump.py`](host/nesdump.py) | Python CLI 吸い出しスクリプト（Python + `pyserial` 環境向け） |
| [`docs/DEVLOG.md`](docs/DEVLOG.md) | **開発日記・トラブルシューティング記録 ＆ AI引き継ぎガイド** |

### ローカル開発環境の専用ツールパス（Windows）
- **Arduino CLI（Arduino IDE 2.x 同梱バイナリ）**：
  ```powershell
  "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
  ```
- **接続シリアルポート**：`COM3` (`115200 bps`)

---

## 🛠️ ハードウェア配線表（最新・実機100%検証済み）

### ⚠️ コネクタ刻印番号（`1〜30`）の向きに関する重要な注意
使用している60ピンエッジコネクタの刻印番号（`A1〜A30`, `B1〜B30`）は：
- **A列（`A1〜A30`）**：カセットの **表側（イラストラベル面）**
- **B列（`B1〜B30`）**：カセットの **裏側（注意書きシール面）**
- ただし左右の番号が **右から左（`FC標準ピン番号 = 31 - コネクタ刻印番号`）** に振られています（**`A30` が FC Pin 1 `GND`**、**`A1` が FC Pin 30 `+5V`**）。
- 以下の表および [`firmware/nesdumper/nesdumper.ino`](firmware/nesdumper/nesdumper.ino) は、すべて **実機コネクタに印字されている刻印（`A1〜A30`, `B1〜B30`）** に完全一致させてあります。
- ※本配線が採用された経緯（ソフト補正の判断・再現性の保証）の詳細は [`docs/DEVLOG.md` の考察・知見 10](docs/DEVLOG.md#考察知見-10ピンアサイン左右反転鏡像関係の経緯ソフト補正の判断および配線の再現性) を参照してください。

### 1. 機能別 ⇔ Arduino Mega 2560 ピン対応早見表（ファームウェア定義順）

#### 電源・制御信号
| 信号名 | 60Pコネクタ刻印 | FC標準ピン | Arduino Mega 2560 刻印 | 役割・備考 |
|---|:---:|:---:|:---:|---|
| **GND** | **A30** | Pin 1 | **GND** | カートリッジ GND（A15/B15ともカセット内部で共通） |
| **+5V** | **A1** | Pin 30 | **5V** | カートリッジ電源 (+5V) |
| **CPU R/W** | **A17** | Pin 14 | **D13** | CPU 読み書き制御（読み出し時 HIGH） |
| **M2 (Φ2)** | **B29** | Pin 32 | **D10** | CPU クロック信号 |
| **/ROMSEL** | **B17** | Pin 44 | **D14** *(D2も同期出力)* | **PRG-ROM チップイネーブル (/CE)** ※必須 |
| **PPU /RD** | **A14** | Pin 17 | **D3** | **CHR-ROM 出力イネーブル (/OE)** ※必須・接触注意 |
| **PPU A13** | **B5** | Pin 56 | **D31** | **CHR-ROM チップイネーブル (/CE)** ※必須 |
| **PPU /A13** | **B12** | Pin 49 | **D48** | PPU A13 反転信号 |

#### CPU / PRG-ROM アドレスバス (`PIN_CPU_A[0..14]`) ＆ データバス (`PIN_CPU_D[0..7]`)
| CPU アドレス | 60P刻印 | Arduinoピン | | CPU データ | 60P刻印 | Arduinoピン |
|---|:---:|:---:|---|---|:---:|:---:|
| **CPU A0** | **A18** | **D22** | | **CPU D0** | **B18** | **D49** |
| **CPU A1** | **A19** | **ANALOG A6** | | **CPU D1** | **B19** | **ANALOG A14** |
| **CPU A2** | **A20** | **ANALOG A5** | | **CPU D2** | **B20** | **ANALOG A7** |
| **CPU A3** | **A21** | **ANALOG A4** | | **CPU D3** | **B21** | **ANALOG A8** |
| **CPU A4** | **A22** | **ANALOG A3** | | **CPU D4** | **B22** | **ANALOG A9** |
| **CPU A5** | **A23** | **ANALOG A2** | | **CPU D5** | **B23** | **ANALOG A10** |
| **CPU A6** | **A24** | **ANALOG A1** | | **CPU D6** | **B24** | **ANALOG A11** |
| **CPU A7** | **A25** | **ANALOG A0** | | **CPU D7** | **B25** | **ANALOG A12** |
| **CPU A8** | **A26** | **D5** | | | | |
| **CPU A9** | **A27** | **D6** | | | | |
| **CPU A10** | **A28** | **D7** | | | | |
| **CPU A11** | **A29** | **D8** | | | | |
| **CPU A12** | **B28** | **D11** | | | | |
| **CPU A13** | **B27** | **D12** | | | | |
| **CPU A14** | **B26** | **ANALOG A13** | | | | |

#### PPU / CHR-ROM アドレスバス (`PIN_PPU_A[0..13]`) ＆ データバス (`PIN_PPU_D[0..7]`)
| PPU アドレス | 60P刻印 | Arduinoピン | | PPU データ | 60P刻印 | Arduinoピン |
|---|:---:|:---:|---|---|:---:|:---:|
| **PPU A0** | **A6** | **D29** | | **PPU D0** | **A5** | **D37** |
| **PPU A1** | **A7** | **D28** | | **PPU D1** | **A4** | **D36** |
| **PPU A2** | **A8** | **D27** | | **PPU D2** | **A3** | **D35** |
| **PPU A3** | **A9** | **D26** | | **PPU D3** | **A2** | **D34** |
| **PPU A4** | **A10** | **D25** | | **PPU D4** | **B1** | **D9** |
| **PPU A5** | **A11** | **D24** | | **PPU D5** | **B2** | **D4** |
| **PPU A6** | **A12** | **D23** | | **PPU D6** | **B3** | **D33** |
| **PPU A7** | **B11** | **D47** | | **PPU D7** | **B4** | **D32** |
| **PPU A8** | **B10** | **D46** | | | | |
| **PPU A9** | **B9** | **D45** | | | | |
| **PPU A10** | **B8** | **D44** | | | | |
| **PPU A11** | **B7** | **D43** | | | | |
| **PPU A12** | **B6** | **D42** | | | | |
| **PPU A13** | **B5** | **D31** | | | | |

---

### 2. 60Pコネクタ番号順（`A1〜A30` / `B1〜B30`）全ピン一覧

#### A面（表側：イラストラベル面 `A1 〜 A30`）
| 60P刻印 | FC標準ピン | FC信号名 | Arduino Mega 2560 刻印 | 役割 |
|:---:|:---:|---|---|---|
| **A1** | Pin 30 | **+5V** | **5V** | カートリッジ電源 (+5V) |
| **A2** | Pin 29 | **PPU D3** | **D34** | CHRデータバス Bit 3 |
| **A3** | Pin 28 | **PPU D2** | **D35** | CHRデータバス Bit 2 |
| **A4** | Pin 27 | **PPU D1** | **D36** | CHRデータバス Bit 1 |
| **A5** | Pin 26 | **PPU D0** | **D37** | CHRデータバス Bit 0 |
| **A6** | Pin 25 | **PPU A0** | **D29** | CHRアドレス Bit 0 |
| **A7** | Pin 24 | **PPU A1** | **D28** | CHRアドレス Bit 1 |
| **A8** | Pin 23 | **PPU A2** | **D27** | CHRアドレス Bit 2 |
| **A9** | Pin 22 | **PPU A3** | **D26** | CHRアドレス Bit 3 |
| **A10** | Pin 21 | **PPU A4** | **D25** | CHRアドレス Bit 4 |
| **A11** | Pin 20 | **PPU A5** | **D24** | CHRアドレス Bit 5 |
| **A12** | Pin 19 | **PPU A6** | **D23** | CHRアドレス Bit 6 |
| **A13** | Pin 18 | CIRAM A10 | *(未接続)* | ミラーリング信号 |
| **A14** | Pin 17 | **PPU /RD** | **D3** | **CHR-ROM 出力許可 (/OE)** ※必須 |
| **A15** | Pin 16 | **GND** | *(未接続 / A30と内部共通)* | グラウンド |
| **A16** | Pin 15 | /IRQ | *(未接続)* | 割り込み出力 |
| **A17** | Pin 14 | **CPU R/W** | **D13** | CPU 読み書き制御 |
| **A18** | Pin 13 | **CPU A0** | **D22** | PRGアドレス Bit 0 |
| **A19** | Pin 12 | **CPU A1** | **ANALOG A6** | PRGアドレス Bit 1 |
| **A20** | Pin 11 | **CPU A2** | **ANALOG A5** | PRGアドレス Bit 2 |
| **A21** | Pin 10 | **CPU A3** | **ANALOG A4** | PRGアドレス Bit 3 |
| **A22** | Pin 9 | **CPU A4** | **ANALOG A3** | PRGアドレス Bit 4 |
| **A23** | Pin 8 | **CPU A5** | **ANALOG A2** | PRGアドレス Bit 5 |
| **A24** | Pin 7 | **CPU A6** | **ANALOG A1** | PRGアドレス Bit 6 |
| **A25** | Pin 6 | **CPU A7** | **ANALOG A0** | PRGアドレス Bit 7 |
| **A26** | Pin 5 | **CPU A8** | **D5** | PRGアドレス Bit 8 |
| **A27** | Pin 4 | **CPU A9** | **D6** | PRGアドレス Bit 9 |
| **A28** | Pin 3 | **CPU A10** | **D7** | PRGアドレス Bit 10 |
| **A29** | Pin 2 | **CPU A11** | **D8** | PRGアドレス Bit 11 |
| **A30** | Pin 1 | **GND** | **GND** | グラウンド |

#### B面（裏側：注意書きシール面 `B1 〜 B30`）
| 60P刻印 | FC標準ピン | FC信号名 | Arduino Mega 2560 刻印 | 役割 |
|:---:|:---:|---|---|---|
| **B1** | Pin 60 | **PPU D4** | **D9** | CHRデータバス Bit 4 |
| **B2** | Pin 59 | **PPU D5** | **D4** | CHRデータバス Bit 5 |
| **B3** | Pin 58 | **PPU D6** | **D33** | CHRデータバス Bit 6 |
| **B4** | Pin 57 | **PPU D7** | **D32** | CHRデータバス Bit 7 |
| **B5** | Pin 56 | **PPU A13** | **D31** | **CHRアドレス Bit 13 (CHR-ROM /CE)** ※必須 |
| **B6** | Pin 55 | **PPU A12** | **D42** | CHRアドレス Bit 12 |
| **B7** | Pin 54 | **PPU A11** | **D43** | CHRアドレス Bit 11 |
| **B8** | Pin 53 | **PPU A10** | **D44** | CHRアドレス Bit 10 |
| **B9** | Pin 52 | **PPU A9** | **D45** | CHRアドレス Bit 9 |
| **B10** | Pin 51 | **PPU A8** | **D46** | CHRアドレス Bit 8 |
| **B11** | Pin 50 | **PPU A7** | **D47** | CHRアドレス Bit 7 |
| **B12** | Pin 49 | **PPU /A13** | **D48** | PPU /A13 反転出力 |
| **B13** | Pin 48 | CIRAM /CE | *(未接続)* | 本体VRAM制御 |
| **B14** | Pin 47 | PPU /WR | *(未接続)* | CHR-RAM 書き込み制御 |
| **B15** | Pin 46 | Audio Out / GND | *(未接続)* | 拡張音源 / GND |
| **B16** | Pin 45 | Audio In | *(未接続)* | 拡張音源入力 |
| **B17** | Pin 44 | **/ROMSEL** | **D14** | **PRG-ROM 出力許可 (/CE)** ※必須 |
| **B18** | Pin 43 | **CPU D0** | **D49** | PRGデータバス Bit 0 |
| **B19** | Pin 42 | **CPU D1** | **ANALOG A14** | PRGデータバス Bit 1 |
| **B20** | Pin 41 | **CPU D2** | **ANALOG A7** | PRGデータバス Bit 2 |
| **B21** | Pin 40 | **CPU D3** | **ANALOG A8** | PRGデータバス Bit 3 |
| **B22** | Pin 39 | **CPU D4** | **ANALOG A9** | PRGデータバス Bit 4 |
| **B23** | Pin 38 | **CPU D5** | **ANALOG A10** | PRGデータバス Bit 5 |
| **B24** | Pin 37 | **CPU D6** | **ANALOG A11** | PRGデータバス Bit 6 |
| **B25** | Pin 36 | **CPU D7** | **ANALOG A12** | PRGデータバス Bit 7 |
| **B26** | Pin 35 | **CPU A14** | **ANALOG A13** | PRGアドレス Bit 14 |
| **B27** | Pin 34 | **CPU A13** | **D12** | PRGアドレス Bit 13 |
| **B28** | Pin 33 | **CPU A12** | **D11** | PRGアドレス Bit 12 |
| **B29** | Pin 32 | **M2** | **D10** | CPUクロック信号 (Φ2) |
| **B30** | Pin 31 | +5V | *(未接続)* | カートリッジ電源 (+5V) |

---

## 🚀 専用ツールの使い方

### 1. ファームウェアのコンパイル＆書き込み (`arduino-cli`)
ファームウェア [`firmware/nesdumper/nesdumper.ino`](firmware/nesdumper/nesdumper.ino) を修正した際は、リポジトリルートから以下のコマンドでコンパイルと `COM3` への書き込みを行います（カセットを挿したまま書き込み可能ですが、シリアルポートを開いているブラウザタブ等は閉じてから実行してください）：

```powershell
$arduinoCli = "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
& $arduinoCli compile --fqbn arduino:avr:mega ./firmware/nesdumper
& $arduinoCli upload -p COM3 --fqbn arduino:avr:mega ./firmware/nesdumper
```

### 2. ブラウザ吸い出しツール (`web/index.html`) 【推奨】
1. Google Chrome または Microsoft Edge で [`web/index.html`](web/index.html) を直接開きます。
2. **「シリアルポート接続」** ボタンを押し、`COM3`（Arduino Mega 2560）を選択します。
3. **「🔍 全ピン接触診断」** を押すと、カセット端子の接触状態（全43本のデータ線・アドレス線の導通、ビット固定、アドレス応答）を自動検査します。接触不良があれば端子番号（例: `B20 / ANALOG A7`）つきで警告表示されます。
4. （配線をやり直した際など）カセット未挿入時に **「⚡ 配線・短絡診断」** を押すと、全ピンのGND短絡およびピン同士のショートを自動診断できます。
5. **「カートリッジ疎通テスト (TEST)」** を押すと、NMI/RESET/IRQベクタと PRG `$C000-$C01F` / CHR `$0000-$001F` の先頭32バイトが表示されます。
6. タイトルプリセットを選択し、**「ROM吸い出し開始 (.nes生成)」** を押します。
7. 吸い出し完了と同時に **PRG / CHR / Combined の CRC32** が表示され、CHRのキャラクタ画像がキャンバスに描画され、`.nes` ファイルがダウンロードされます。

### 3. PowerShell からの直接吸い出し・CRC32検証ワンライナー
Python不要で、Windows標準の PowerShell から直接 `COM3` と通信して吸い出し＆CRC32検証を行う手順です（AIエージェントが自動テストを行う際もこのスクリプトを使用します）：

```powershell
$port = New-Object System.IO.Ports.SerialPort("COM3", 115200, "None", 8, "One")
$port.DtrEnable = $true
$port.ReadTimeout = 5000
$port.Open()
Start-Sleep -Milliseconds 2200
$port.DiscardInBuffer()

function Read-Exact($port, [int]$total) {
    $buf = New-Object byte[] $total
    $read = 0
    while ($read -lt $total) { $read += $port.Read($buf, $read, $total - $read) }
    return $buf
}

# PRG-ROM 16KB ($C000-$FFFF): ヘッダ "OK 4000\r\n" (9B) + データ (16384B) + フッタ "CRC XXXXXXXX\r\n" (14B)
$port.WriteLine("R C000 4000")
$rawPrg = Read-Exact $port (9 + 16384 + 14)
$prgBytes = $rawPrg[9..(9 + 16383)]
$prgCrc = [System.Text.Encoding]::ASCII.GetString($rawPrg, 9 + 16384, 14).Trim()

Start-Sleep -Milliseconds 50
$port.DiscardInBuffer()

# CHR-ROM 8KB ($0000-$1FFF): ヘッダ "OK 2000\r\n" (9B) + データ (8192B) + フッタ "CRC XXXXXXXX\r\n" (14B)
$port.WriteLine("C 0000 2000")
$rawChr = Read-Exact $port (9 + 8192 + 14)
$chrBytes = $rawChr[9..(9 + 8191)]
$chrCrc = [System.Text.Encoding]::ASCII.GetString($rawChr, 9 + 8192, 14).Trim()
$port.Close()

Write-Output "PRG Result: $prgCrc"
Write-Output "CHR Result: $chrCrc"

# iNESファイル生成 (Mapper 0, 16KB PRG, 8KB CHR, Vertical Mirroring)
$header = [byte[]](0x4E, 0x45, 0x53, 0x1A, 0x01, 0x01, 0x01, 0x00, 0,0,0,0,0,0,0,0)
$nes = New-Object byte[] (16 + 16384 + 8192)
[Array]::Copy($header, 0, $nes, 0, 16)
[Array]::Copy($prgBytes, 0, $nes, 16, 16384)
[Array]::Copy($chrBytes, 0, $nes, 16400, 8192)
[System.IO.File]::WriteAllBytes("./ninjakun_majou.nes", $nes)
```

### 4. ファームウェアのシリアルコマンド一覧 (`115200 bps`, 改行 `\n`)
| コマンド | 動作・応答フォーマット |
|---|---|
| `PING` | 疎通確認。`PONG\r\n` を返す |
| `SCAN` | 全ピンを `INPUT_PULLUP` にして `GND` に落ちているピン番号を一覧表示する安全診断コマンド |
| `PINCHECK` | カセット挿入時：全データ線・アドレス線の導通、ビット固定（常時HIGH/LOW）、アドレスエイリアスを診断 |
| `WIRECHECK` | カセット未挿入時：全50ピンのGND短絡およびピン間ショート（誤接続）を全組み合わせで網羅診断 |
| `PROBE` | 配線プローブモード（ピンをGNDにタッチすると即座にピン名・コネクタ端子番号をシリアル出力） |
| `TEST` | NMI/RESET/IRQベクタ、PRG `$C000-$C01F`、CHR `$0000-$001F` を16進ダンプ表示 |
| `R <addr_hex> <len_hex>` | **PRG-ROM 読み出し**。`OK <len_hex>\r\n` ＋ バイナリ `<len>` バイト ＋ `CRC <8桁HEX>\r\n` を返す |
| `C <addr_hex> <len_hex>` | **CHR-ROM 読み出し**。`OK <len_hex>\r\n` ＋ バイナリ `<len>` バイト ＋ `CRC <8桁HEX>\r\n` を返す |
| `W <addr_hex> <len_hex>` | WRAM（`$6000-$7FFF`）読み出し |
| `B <addr_hex>` | バス競合回避付きバンク切り替え書き込み（CNROM等でROM上の値と同じデータで書き込む際） |
| `P <addr_hex> <data_hex>` | CPUアドレス空間への1バイト書き込み（マッパーレジスタ制御用） |

---

## 📚 リファレンス・関連資料
- [開発日記・トラブルシューティング記録 ＆ AI引き継ぎガイド (`docs/DEVLOG.md`)](docs/DEVLOG.md)
- [NESdev Wiki - Cartridge connector (Famicom 60-pin)](https://www.nesdev.org/wiki/Cartridge_connector)
- [GOROman/nes-rom-reader - docs/hardware-design.md](https://github.com/GOROman/nes-rom-reader/blob/main/docs/hardware-design.md)

---

## 📄 ライセンス・免責事項

本プロジェクトは [MIT License](LICENSE) の下で公開されています。

### ⚠️ 免責事項（Disclaimer）
- 本ソフトウェア、回路配線情報、およびドキュメントは、研究・学習および個人の電子工作を目的として「現状有姿（AS IS）」で提供されます。
- 作者および権利者は、本ツールの使用、誤配線、ショート、静電気等により生じたいかなる損害（マイコンボードやカートリッジの破損・故障・データ消失等を含む）についても一切の責任を負いません。すべてご自身の責任においてご使用ください。
- カートリッジの吸い出しは、ご自身が合法的に所有するカートリッジの個人利用・バックアップ目的でのみ実施し、著作権法および関係法令を遵守してください。
