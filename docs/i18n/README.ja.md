# E-Ink Assistant

<!-- BEGIN README LANGUAGES -->
<p align="center">
  <a href="../../README.md" lang="en" dir="ltr">English</a> &nbsp;·&nbsp;
  <a href="README.zh-Hans.md" lang="zh-Hans" dir="ltr">简体中文</a> &nbsp;·&nbsp;
  <a href="README.zh-Hant.md" lang="zh-Hant" dir="ltr">繁體中文</a> &nbsp;·&nbsp;
  <b lang="ja" dir="ltr">日本語</b> &nbsp;·&nbsp;
  <a href="README.ko.md" lang="ko" dir="ltr">한국어</a> &nbsp;·&nbsp;
  <a href="README.es.md" lang="es" dir="ltr">Español</a> &nbsp;·&nbsp;
  <a href="README.fr.md" lang="fr" dir="ltr">Français</a> &nbsp;·&nbsp;
  <a href="README.de.md" lang="de" dir="ltr">Deutsch</a> &nbsp;·&nbsp;
  <a href="README.pt-BR.md" lang="pt-BR" dir="ltr">Português (Brasil)</a> &nbsp;·&nbsp;
  <a href="README.ru.md" lang="ru" dir="ltr">Русский</a> &nbsp;·&nbsp;
  <a href="README.ar.md" lang="ar" dir="rtl">العربية</a> &nbsp;·&nbsp;
  <a href="README.hi.md" lang="hi" dir="ltr">हिन्दी</a>
</p>
<!-- END README LANGUAGES -->

<p align="center">
  <img src="../../Resources/AppIcon.png" alt="E-Ink Assistant のアプリアイコン" width="128">
</p>

**macOS と Windows で、白黒・カラー電子ペーパーディスプレイを調整。**

[製品ウェブサイトを見る](https://kiteretsu903.github.io/eink-assistant/ja/)

E-Ink Assistant は、選択した電子ペーパーディスプレイのテキストコントラスト、暗部の階調、色を調整します。ほかのディスプレイは変更しません。macOS 版はメニューバーに、Windows 版はシステムトレイに常駐します。

[macOS 2.6 をダウンロード](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-v2.6.dmg) ·
[Windows 1.2 をダウンロード](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-Windows-1.2-Setup.exe) ·
[すべてのリリースを見る](https://github.com/kiteretsu903/eink-assistant/releases)

無料・オープンソース。MIT ライセンスで公開しています。

## 機能とシステム要件

| 機能 | macOS | Windows |
|---|---|---|
| 対応システム | **macOS 14 以降**<br>Apple silicon のみ | **Windows 7 SP1 から Windows 11 まで**<br>x64 コンピューター |
| アプリの常駐場所 | メニューバー | システムトレイ |
| 特定の電子ペーパーディスプレイを選択 | 対応。ほかのディスプレイは変更しません。 | macOS と同じ |
| テキストコントラスト | 4 段階：中、強、シャープ、ソリッド | macOS と同じ |
| 映像の暗部補正 | 3 段階：弱、中、強 | macOS と同じ |
| 詳細カーブとプリセット | リアルタイムのカーブ編集と 5 個の名前付きプリセット | macOS と同じ |
| 彩度と RGB | ディスプレイ別カラープロファイル。彩度 0%–300%、RGB 0%–200% | 対応条件を満たす Windows 10 2004 および Windows 11 21H2+ で利用可能。利用できる方式はシステムとハードウェアによって異なります |
| ちらつきを抑える | 対応する外付けディスプレイで利用可能。電子ペーパーとして選択するとオンになります | 非対応。Windows には統一された公開のディスプレイ別ディザリング制御がなく、多くの Windows 環境ではおそらく必要ありません。 |
| 透明度を下げる・視差効果を減らす | 初回にユーザーが確認する補助ショートカットで利用可能 | Windows 7 SP1 以降で、対応するシステム API を通じて利用可能 |
| システムのライトモード | 変更しません | Windows 10 1903+ で、アプリ実行中のみ Windows ライトモードを適用 |
| Night Shift / 夜間モード | ディスプレイ別に Night Shift と True Tone の対象から除外。管理者の承認と再接続が必要 | Windows 10 1703+ では夜間モード設定を開けます。Windows 11 24H2+ では夜間モードを直接無効化できます |
| ミラーリング／複製表示 | ミラーリングされた物理ディスプレイも個別に選択可能 | トーンカーブは共有の出力元に作用します。彩度と RGB には拡張表示モードが必要です |
| 変更を元に戻す | 一時的なカーブ、カラープロファイル、ディザリングは終了時に復元。Night Shift / True Tone の除外設定は保持されます | 一時的なガンマ、色、視覚効果、夜間モードの変更は終了時に復元。色と夜間モードは異常終了後にも復元されます |
| ログイン時に起動 | 対応 | 対応 |
| 表示言語 | 英語、簡体字中国語、繁体字中国語、日本語 | macOS と同じ |
| 管理者権限 | 任意の Night Shift / True Tone 除外設定にのみ必要 | インストーラーとアプリで必要 |

[macOS 版の詳細](../../macos/README.md) ·
[Windows の互換性と設定](../../WINDOWS.md) ·
[macOS 版の変更履歴](../../CHANGELOG.md) ·
[Windows 版の変更履歴](../../windows/CHANGELOG.md)

<p align="center">
  <img src="../../docs/en/app-main-v2-1.png" alt="E-Ink Assistant v2.1（英語表示）" width="440">
</p>

## 調整機能

| 機能 | 主な用途 | 効果 |
|---|---|---|
| テキストコントラスト | 読書 | 「中」「強」「シャープ」「ソリッド」の各段階で薄い文字を濃くします。強くするほどグレーの階調が減り、輪郭がくっきりします。 |
| 映像の暗部補正 | 写真・動画 | 「弱」「中」「強」の各段階で暗部の細部を見やすくします。濃い文字も薄くなるため、読書時にはオフにしてください。 |
| 彩度と RGB | カラー電子ペーパー | 対応するプラットフォームでは、6 種類の彩度プリセット、0%–300% の彩度スライダー、0%–200% の RGB 補正を利用できます。 |
| ちらつきを抑える | 対応する macOS ディスプレイ | ディザリングによる目に見えるちらつきを止めます。電子ペーパーとして選択したディスプレイでは自動的にオンになります。 |
| Night Shift と True Tone | 色温度の変化を受けるディスプレイ | 選択した macOS ディスプレイを対象から除外します。管理者の承認とディスプレイの再接続が必要で、設定は終了後も保持されます。 |
| 透明度を下げる・視差効果を減らす | 更新の遅いパネル | システムの視覚効果を簡素化します。macOS では、初回にユーザーが確認する補助ショートカットを使います。 |
| 詳細カーブ | パネルごとの微調整 | リアルタイムのグラフと 5 個の名前付きプリセットを使って、ニー、ガンマ、ブラックポイント、ホワイトポイントを調整します。 |

<p align="center">
  <img src="../../docs/en/text-contrast-editorial.png" alt="テキストコントラスト調整前後を示すイメージ" width="31%">
  <img src="../../docs/en/video-enhance-editorial.png" alt="映像の暗部補正前後を示すイメージ" width="31%">
  <img src="../../docs/en/saturation-editorial.png" alt="彩度調整前後を示すイメージ" width="31%">
</p>

> これらの画像は調整機能の効果を示すイメージです。結果はパネルと表示内容によって異なります。

## インストール

### macOS 14+、Apple silicon

1. 上のリンクから macOS 2.6 の DMG をダウンロードします。
2. DMG を開き、**E-Ink Assistant** を **アプリケーション** にドラッグします。
3. まず一度アプリを開いてみてください。macOS にブロックされた場合は、**システム設定 → プライバシーとセキュリティ**を開き、**このまま開く**を選択します。

個人開発のソフトウェアで、現在 App Store では配信していません。初回起動時には macOS に「検証できない」という警告が表示されます。コードはすべて公開されているため、利用するかどうかを決める前に確認できます。

アプリを「アプリケーション」に移動しても **このまま開く** が表示されない場合は、ターミナルを開いて次を実行します。

```
xattr -dr com.apple.quarantine "/Applications/E-Ink Assistant.app"
```

### Windows 7 SP1 から Windows 11 まで、x64

1. 上のリンクから Windows 1.2 のインストーラーをダウンロードします。
2. セットアップを実行し、管理者権限の確認を承認します。
3. スタートメニューまたはシステムトレイから E-Ink Assistant を開きます。

Windows のバージョン、GPU、ドライバー、ディスプレイの接続方式ごとの正確な対応状況は、[WINDOWS.md](../../WINDOWS.md) を参照してください。

## 使い方

<p align="center">
  <img src="../../docs/en/app-displays-v2-1.png" alt="E-Ink Assistant v2.1 でのディスプレイ選択（英語表示）" width="440">
</p>

1. macOS のメニューバーまたは Windows のシステムトレイからアプリを開きます。
2. 調整したい白黒・カラー電子ペーパーディスプレイをそれぞれ選択します。
3. まず、モニター本体のメニューでバランスの取れたハードウェアコントラストに設定します。
4. 読書にはテキストコントラスト、写真や動画には映像の暗部補正を選びます。同時には使いません。
5. カラー電子ペーパーでは、対応するプラットフォームで彩度と RGB を調整します。

**ディスプレイの調整はアプリ終了時に元に戻り**、起動時に再適用されます。自動起動するには **ログイン時に起動** をオンにしてください。

## ディスプレイの設定

アプリで調整する前に、モニター本体のメニューでバランスの取れたコントラストに設定してください。付属プリセットは、**Bigme B251 Pro**（R2 FW V2.0）を **Web Mode、Hardware Gamma Level 3、Contrast 50、Color Restore Mode off** に設定して、目視で調整したものです。白黒パネルや別のカラーモデルでは、それぞれに合った値が必要です。詳細モードではカーブ全体を調整でき、設定はディスプレイごとに保存されます。

ちらつきを抑える機能は Apple Silicon のみ対応し、非対応の環境では表示されません。

<details>
<summary>macOS の「透明度を下げる・視差効果を減らす」補助ショートカット</summary>

初回利用時には、Apple の「ショートカット」アプリで **ショートカットを追加** を確認します。付属の補助ショートカットは、完全一致するテキストコマンド `on` と `off` だけを受け取り、出力を返しません。共有シート、Spotlight、クイックアクション、ロック画面のインターフェイスには表示されず、Mac のロック中にも実行できます。アプリがほかのショートカットを一覧表示したり、調べたりすることはありません。

自動モードでは、選択した電子ペーパーディスプレイを接続すると両方の設定がオンになり、最後の 1 台を切断するとオフになります。アプリを終了した場合も、両方の設定をオフにします。

</details>

## プロジェクトのドキュメント

- [CHANGELOG.md](../../CHANGELOG.md)：各バージョンの変更内容
- [TECHNICAL.md](../../TECHNICAL.md)：実装、測定結果、現在の macOS では*機能しない*手法
- [mac-saturation](https://github.com/kiteretsu903/mac-saturation)：カラー調整の仕組みの調査と、プロファイルを書き出す CLI

## ライセンスとクレジット

MIT。[LICENSE](../../LICENSE) を参照してください。

**ちらつきを抑える機能は、Abdullah Arif 氏の [Stillcolor](https://github.com/aiaf/Stillcolor)（MIT）を基にしています。** Stillcolor は、I/O Registry の `enableDither` プロパティでディスプレイのディザリングを無効にできることを発見しました。このプロジェクトでは、その手法をディスプレイごとに適用する形で再実装しています。発見の功績は Stillcolor にあります。ありがとうございます。

すべての通知は [THIRD-PARTY-NOTICES.md](../../THIRD-PARTY-NOTICES.md) に記載しています。
