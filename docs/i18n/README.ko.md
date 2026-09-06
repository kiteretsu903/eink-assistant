# E-Ink Assistant

<!-- BEGIN README LANGUAGES -->
<p align="center">
  <a href="../../README.md" lang="en" dir="ltr">English</a> &nbsp;·&nbsp;
  <a href="README.zh-Hans.md" lang="zh-Hans" dir="ltr">简体中文</a> &nbsp;·&nbsp;
  <a href="README.zh-Hant.md" lang="zh-Hant" dir="ltr">繁體中文</a> &nbsp;·&nbsp;
  <a href="README.ja.md" lang="ja" dir="ltr">日本語</a> &nbsp;·&nbsp;
  <b lang="ko" dir="ltr">한국어</b> &nbsp;·&nbsp;
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
  <img src="../../Resources/AppIcon.png" alt="E-Ink Assistant 앱 아이콘" width="128">
</p>

**macOS와 Windows에서 흑백 및 컬러 전자잉크 디스플레이를 조정하세요.**

[제품 웹사이트 방문](https://kiteretsu903.github.io/eink-assistant/ko/)

E-Ink Assistant는 선택한 전자잉크 디스플레이의 텍스트 명암, 어두운 영역의 세부 표현, 색상을 조정합니다. 다른 디스플레이는 바뀌지 않습니다. macOS 버전은 메뉴 막대에서, Windows 버전은 시스템 트레이에서 실행됩니다.

[macOS 2.6 다운로드](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-v2.6.dmg) ·
[Windows 1.2 다운로드](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-Windows-1.2-Setup.exe) ·
[모든 릴리스 보기](https://github.com/kiteretsu903/eink-assistant/releases)

무료 오픈 소스이며 MIT 라이선스로 제공됩니다.

## 기능 및 시스템 요구 사항

| 기능 | macOS | Windows |
|---|---|---|
| 지원 시스템 | **macOS 14 이상**<br>Apple silicon 전용 | **Windows 7 SP1부터 Windows 11까지**<br>x64 컴퓨터 |
| 앱 실행 위치 | 메뉴 막대 | 시스템 트레이 |
| 특정 전자잉크 디스플레이 선택 | 가능합니다. 다른 디스플레이는 바뀌지 않습니다. | macOS와 동일 |
| 텍스트 명암 | 네 단계: 중간, 강하게, 선명하게, 진하게 | macOS와 동일 |
| 영상 개선 | 세 단계: 약하게, 중간, 강하게 | macOS와 동일 |
| 고급 곡선 및 프리셋 | 실시간 곡선 편집기와 이름을 지정할 수 있는 프리셋 다섯 개 | macOS와 동일 |
| 채도 및 RGB | 디스플레이별 색상 프로필, 채도 0%–300%, RGB 0%–200% | 지원 조건을 충족하는 Windows 10 2004 및 Windows 11 21H2+ 시스템에서 사용 가능하며, 사용 방식은 시스템과 하드웨어에 따라 다름 |
| 깜박임 줄이기 | 지원되는 외장 디스플레이에서 사용 가능하며, 전자잉크 디스플레이로 선택하면 켜짐 | 사용할 수 없습니다. Windows에는 통일된 디스플레이별 공개 디더링 제어 기능이 없으며, 대부분의 Windows 시스템에서는 필요하지 않을 가능성이 높습니다. |
| 투명도 및 동작 줄이기 | 사용자가 한 번 확인하여 설치하는 도우미를 통해 사용 가능 | Windows 7 SP1부터 호환되는 시스템 API를 통해 사용 가능 |
| 시스템 라이트 모드 | 변경하지 않음 | Windows 10 1903+에서 앱 실행 중에만 Windows 라이트 모드 적용 |
| Night Shift / 야간 모드 | 디스플레이별 Night Shift 및 True Tone 제외, 관리자 승인 및 재연결 필요 | Windows 10 1703+에서 야간 모드 설정 제공, Windows 11 24H2+에서 야간 모드 직접 끄기 가능 |
| 미러링 / 복제 디스플레이 | 미러링된 물리 디스플레이도 개별 선택 가능 | 색조 곡선은 공유된 출력 소스에 영향을 주며, 채도와 RGB에는 확장 모드가 필요 |
| 변경 사항 복원 | 임시 곡선, 색상 프로필, 디더링은 종료 시 복원되지만 Night Shift / True Tone 제외 설정은 유지됨 | 임시 감마, 색상, 시각 효과, 야간 모드 변경은 종료 시 복원되며, 색상과 야간 모드는 비정상 종료 후에도 복구됨 |
| 로그인 시 실행 | 지원 | 지원 |
| 인터페이스 언어 | 영어, 중국어 간체, 중국어 번체, 일본어 | macOS와 동일 |
| 관리자 권한 | 선택 사항인 Night Shift / True Tone 제외 기능에만 필요 | 설치 프로그램과 앱에 필요 |

[macOS 상세 정보](../../macos/README.md) ·
[Windows 호환성 및 설정](../../WINDOWS.md) ·
[macOS 변경 기록](../../CHANGELOG.md) ·
[Windows 변경 기록](../../windows/CHANGELOG.md)

<p align="center">
  <img src="../../docs/en/app-main-v2-1.png" alt="영어로 표시된 E-Ink Assistant v2.1" width="440">
</p>

## 조절 기능

| 조절 기능 | 용도 | 동작 |
|---|---|---|
| 텍스트 명암 | 읽기 | 중간, 강하게, 선명하게, 진하게 단계로 희미한 글자를 어둡게 합니다. 강한 단계일수록 회색 세부 표현이 줄어드는 대신 가장자리가 더 뚜렷해집니다. |
| 영상 개선 | 사진과 동영상 | 약하게, 중간, 강하게 단계로 어두운 영역의 세부 표현을 드러냅니다. 어두운 글자도 밝아지므로 읽을 때는 끄세요. |
| 채도 및 RGB | 컬러 전자잉크 | 플랫폼이 지원하는 경우 채도 프리셋 여섯 개, 0%–300% 채도 슬라이더, 0%–200% RGB 보정을 제공합니다. |
| 깜박임 줄이기 | 지원되는 macOS 디스플레이 | 눈에 보이는 디더링의 반짝임을 멈추며, 전자잉크로 선택한 디스플레이에서 자동으로 켜집니다. |
| Night Shift 및 True Tone | 색온도 변화의 영향을 받는 디스플레이 | 선택한 macOS 디스플레이를 적용 대상에서 제외합니다. 관리자 승인과 디스플레이 재연결이 필요하며, 종료 후에도 설정이 유지됩니다. |
| 투명도 및 동작 줄이기 | 새로 고침이 느린 패널 | 시스템 시각 효과를 단순화합니다. macOS에서는 사용자가 한 번 확인하여 설치하는 도우미를 사용합니다. |
| 고급 곡선 | 패널별 세부 조정 | 실시간 그래프와 이름을 지정할 수 있는 프리셋 다섯 개를 통해 꺾임점, 감마, 블랙 포인트, 화이트 포인트를 조정합니다. |

<p align="center">
  <img src="../../docs/en/text-contrast-editorial.png" alt="텍스트 명암 조정 전후 예시" width="31%">
  <img src="../../docs/en/video-enhance-editorial.png" alt="영상 개선 전후 예시" width="31%">
  <img src="../../docs/en/saturation-editorial.png" alt="채도 조정 전후 예시" width="31%">
</p>

> 이 이미지는 조절 기능을 설명하기 위한 예시입니다. 결과는 패널과 원본 콘텐츠에 따라 다릅니다.

## 설치

### macOS 14+, Apple silicon

1. 위 링크에서 macOS 2.6 DMG를 다운로드하세요.
2. 파일을 열고 **E-Ink Assistant**를 **응용 프로그램**으로 드래그하세요.
3. 앱을 한 번 열어 보세요. macOS가 차단하면 **시스템 설정 → 개인정보 보호 및 보안**에서 **확인 없이 열기**를 선택하세요.

이 소프트웨어는 독립적으로 개발되었으며 현재 App Store에 등록되어 있지 않습니다. 처음 열 때 macOS에서 ‘확인할 수 없음’ 경고가 표시됩니다. 코드는 모두 공개되어 있으므로 사용 여부를 결정하기 전에 검토할 수 있습니다.

앱을 응용 프로그램으로 옮긴 뒤에도 **확인 없이 열기**가 나타나지 않으면 터미널을 열어 실행하세요:

```
xattr -dr com.apple.quarantine "/Applications/E-Ink Assistant.app"
```

### Windows 7 SP1부터 Windows 11까지, x64

1. 위 링크에서 Windows 1.2 설치 프로그램을 다운로드하세요.
2. 설치 프로그램을 실행하고 관리자 권한 요청을 승인하세요.
3. 시작 메뉴 또는 시스템 트레이에서 E-Ink Assistant를 여세요.

Windows 버전, GPU, 드라이버, 디스플레이 연결 방식에 따른 정확한 기능 지원 범위는 [WINDOWS.md](../../WINDOWS.md)를 참조하세요.

## 사용 방법

<p align="center">
  <img src="../../docs/en/app-displays-v2-1.png" alt="E-Ink Assistant v2.1에서 디스플레이 선택" width="440">
</p>

1. macOS 메뉴 막대 또는 Windows 시스템 트레이에서 앱을 여세요.
2. 조정할 흑백 또는 컬러 전자잉크 디스플레이를 각각 선택하세요.
3. 먼저 모니터 자체 메뉴에서 하드웨어 명암을 균형 잡힌 수준으로 설정하세요.
4. 읽을 때는 텍스트 명암을, 미디어를 볼 때는 영상 개선을 선택하세요. 두 기능을 함께 사용하지 마세요.
5. 컬러 전자잉크에서는 플랫폼이 지원하는 경우 채도와 RGB를 조정하세요.

**종료하면 디스플레이 조정이 원래대로 복원되며**, 실행하면 다시 적용됩니다. 자동으로 시작하려면 **로그인 시 실행**을 켜세요.

## 디스플레이 설정

앱에서 조정하기 전에 모니터 자체 메뉴에서 명암을 균형 잡힌 수준으로 설정하세요. 기본 제공 프리셋은 **Bigme B251 Pro** (R2 FW V2.0)를 **웹 모드, 하드웨어 Gamma 3단계, 명암 50, 색상 복원 모드 끄기**로 설정하고 눈으로 확인하며 조정했습니다. 흑백 패널이나 다른 컬러 모델에는 별도의 값이 필요합니다. 고급 모드에서는 전체 곡선을 조절할 수 있으며, 설정은 디스플레이별로 따로 저장됩니다.

깜박임 줄이기는 Apple Silicon 전용이며 지원되지 않는 환경에서는 숨겨집니다.

<details>
<summary>macOS 투명도 및 동작 줄이기 도우미</summary>

처음 사용할 때 Apple의 단축어 앱에서 **단축어 추가**를 확인해야 합니다. 기본 제공 도우미는 정확히 `on`과 `off` 텍스트 명령만 입력받고 출력을 만들지 않으며, 공유 시트, Spotlight, 빠른 동작, 잠금 화면 인터페이스에는 표시되지 않습니다. Mac이 잠겨 있어도 실행할 수 있습니다. 앱은 다른 단축어를 나열하거나 검사하지 않습니다.

자동 모드는 선택한 전자잉크 디스플레이가 연결되면 두 설정을 모두 켜고, 선택한 마지막 디스플레이의 연결이 해제되면 끕니다. 앱을 종료해도 두 설정이 모두 꺼집니다.

</details>

## 프로젝트 문서

- [CHANGELOG.md](../../CHANGELOG.md): 버전별 변경 사항
- [TECHNICAL.md](../../TECHNICAL.md): 구현, 측정 결과, 최신 macOS에서 작동하지 *않는* 접근 방식
- [mac-saturation](https://github.com/kiteretsu903/mac-saturation): 색상 처리 방식 조사 및 프로필 내보내기 CLI

## 라이선스 및 기여 출처

MIT 라이선스입니다. [LICENSE](../../LICENSE)를 참조하세요.

**깜박임 줄이기는 Abdullah Arif의 [Stillcolor](https://github.com/aiaf/Stillcolor)를 기반으로 합니다** (MIT). Stillcolor는 `enableDither` I/O Registry 속성으로 디스플레이 디더링을 끌 수 있다는 사실을 발견했습니다. 이 프로젝트는 해당 아이디어를 디스플레이별로 다시 구현한 것으로, 발견에 대한 공로는 Stillcolor에 있습니다. 감사합니다.

전체 고지는 [THIRD-PARTY-NOTICES.md](../../THIRD-PARTY-NOTICES.md)에 있습니다.
