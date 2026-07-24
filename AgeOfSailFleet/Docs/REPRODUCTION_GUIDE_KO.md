# Age of Sail: Fleet Command — 재현 가이드

## 1. 문서 목적과 제작 이력

이 문서는 `AgeOfSailFleet` 프로토타입을 빈 Unreal 프로젝트에서 다시
구현하려는 개발자를 위한 가이드다. 최종 결과만 설명하지 않고 작업 순서,
소스 계약, 자동화, 아키텍처, 검증 기준, 반복 작업에서 발견된 실패 원인을
정리한다.

프로젝트 소유자가 제공한 제작 기록은 다음과 같다.

- 프로젝트 소스, 설정, 자동화, 통합 작업은 외부에서 직접 손으로 작성한
  것이 아니라 Codex 대화를 통해 작성됐다.
- 외부 DCC 도구는 Blender만 사용했으며 Codex가 Blender MCP를 통해
  제어했다.
- 래스터 콘셉트, 텍스처, 아틀라스, 초상화, UI 이미지, 플립북은
  ImageGen으로 만든 뒤 Codex가 작성한 도구로 후처리하고 임포트했다.
- 작업 시작 시 Sol 모델의 추론 수준은 `XHigh`였고 이후 `High`로
  진행했다.

위 네 항목은 Git 메타데이터로 복원한 사실이 아니라 프로젝트 소유자의
작업 기록이다. Git으로 확인되는 구현 이력은 13절에 별도로 정리했다.

## 2. 재현 가능 범위

저장소에는 편집 가능한 Blender 원본, FBX, 원본·후처리 PNG, C++ 소스,
Unreal Python 자동화, 체크인된 Unreal 에셋이 있다. 그러나 Blender MCP
명령 전체 기록, ImageGen 원문 프롬프트, seed, 모델의 숨은 상태는 없다.

따라서 다음 기준으로 이해해야 한다.

- 기능과 시각 방향이 동등한 재현은 가능하다.
- 저장된 소스 에셋으로 현재 프로젝트를 다시 빌드하는 것은 가능하다.
- ImageGen 픽셀 단위 동일 결과나 완전히 같은 Blender 작업 이력은
  보장할 수 없다.
- `*_Source.png`와 `.blend`는 감사·편집용 원본이고, 가공 PNG, FBX,
  `.uasset`, `.umap`은 파이프라인 산출물이다.

아래 ImageGen 프롬프트 설명은 원문 보존본이 아니라 제작 규격을 재구성한
템플릿이다.

## 3. 최종 구현 목표

재현 결과에는 다음 요소가 있어야 한다.

- 바다 위 타이틀, 3:1 영화식 로고, 이미지 모양의 출항 버튼, 상호 배타적인
  둥근 `3D`/`2D` 라디오 버튼, 타이틀 페이드 → 암전 → 전투 페이드 전환
- 파랑 3척 대 빨강 4척, 기함 사이 초기 거리 32,000 Unreal unit
- 1급 전열함, 2급 전열함, 중프리깃의 서로 다른 3개 함급
- 단일·드래그·추가·전체 선택, 우클릭 대형 이동과 공격 명령
- 풍향 기반 속도, 측면 포격 위치 선정, 충돌 회피, 포탄 탄도, 피해,
  파괴 효과와 침몰
- 절차적 바다, 이중 파도 노멀, 항적, 총구 화염, 선체 피격, 수중 피탄,
  크게 보이는 포탄과 탄도 트레일
- Blender로 만든 3D 표현과 진영별 8방향 2D 스프라이트 모드
- 동일 타원 구멍을 공유하는 함장 초상화, Bronze/Silver/Gold 로킷 프레임,
  함장명·함선명, 이미지 전용 날짜 활자, 풍향 화살표 HUD
- `C` 키 전략 카메라·자유 비행 토글

## 4. 필요 환경

- Windows 10/11
- Unreal Engine 5.7
- Visual Studio 2022, Desktop development with C++, Windows SDK, Unreal 호환
  MSVC toolchain
- 체크인된 `.blend`를 열 수 있는 Blender
- 원래 모델링 작업 흐름을 재현할 경우 Blender MCP가 연결된 Codex
- 래스터 에셋을 다시 만들 경우 ImageGen
- 오프라인 아틀라스·마스크 도구용 Python 3.11+와 Pillow

Unreal 프로젝트에서 활성화할 플러그인:

- `ProceduralMeshComponent`
- `PythonScriptPlugin`
- `EditorScriptingUtilities`

모듈 의존성은 `Core`, `CoreUObject`, `Engine`, `InputCore`,
`ProceduralMeshComponent`, `UMG`, `Slate`, `SlateCore`이며 에디터 전용으로
`UMGEditor`, `UnrealEd`를 사용한다.

## 5. 소스 기준 폴더

```text
AgeOfSailFleet/
  Art/
    Source/                 편집 가능한 Blender 마스터
    Preview/                프리뷰 전용 3점 조명 렌더
  Config/                   맵, 게임 모드, 렌더러, 입력 설정
  Content/
    Raw/                    FBX와 원본·가공 래스터
    Python/                 Unreal Editor 임포트·저작 자동화
    Art/                    메시, 머티리얼, 텍스처, 스프라이트, FX
    UI/                     UI 텍스처, 폰트, 실제 WBP
    Maps/FleetOcean.umap    최소 런타임 맵
  Source/AgeOfSailFleet/    네이티브 런타임과 에디터 브리지
  Tools/                    Pillow 슬라이싱·정리 도구
```

주요 모델 소스:

- `Art/Source/AgeOfSailWarship.blend`
- `Art/Source/AgeOfSailFleetVariants.blend`
- `Content/Raw/AgeOfSailWarship_FirstRate.fbx`
- `Content/Raw/AgeOfSailWarship_SecondRate.fbx`
- `Content/Raw/AgeOfSailWarship_Frigate.fbx`
- 안전 대체용 `Content/Raw/AgeOfSailWarship.fbx`

## 6. 권장 구현 순서

### 단계 A — 설계 계약 고정

코드를 작성하기 전에 다음을 불변 조건으로 선언한다.

1. 함선은 충각이 아니라 거리를 유지한 측면 포격을 기본 전술로 사용한다.
2. 세 함급은 메시, 충돌 크기, 능력치, 실루엣, 표현 스케일이 달라야 한다.
3. 파랑·빨강 진영은 돛 색과 문양만 보아도 구분돼야 한다.
4. 기함은 선미, 금장, 페넌트, 갤러리, 랜턴, 왕관·선수상, 지휘 장식이 더
   화려해야 한다.
5. 바람이 실제 이동에 영향을 주고 HUD에도 표시돼야 한다.
6. 누적 피해가 일정 수준을 넘으면 가시적인 침몰이 시작돼야 한다.
7. UMG 전체 계층은 WBP 에셋 안에 존재해야 한다. 네이티브 생성자와
   `NativeConstruct`에서 위젯 트리를 만들지 않는다.
8. Blender 프리뷰 카메라와 조명은 Unreal로 내보내지 않는다.

### 단계 B — UE C++ 골격 작성

Unreal Engine 5.7 C++ 게임 `AgeOfSailFleet`를 만들고 위 플러그인과 모듈
의존성을 추가한다. 다음을 설정한다.

- 기본·에디터 맵: `/Game/Maps/FleetOcean`
- 전역 게임 모드: `/Script/AgeOfSailFleet.SailGameMode`
- DX12, Lumen GI·Reflection, Mesh Distance Field

네이티브 계층은 다음 의존 순서가 안전하다.

1. `ACannonballActor`
2. `ASailShip`
3. `AFleetBattleDirector`, `ASailOceanActor`
4. `AShipWakeActor`, `AFlipbookEffectActor`
5. `AFleetCameraPawn`, `AFleetPlayerController`
6. `USailFleetHUDWidget`, `USailTitleScreenWidget`
7. `ASailGameMode`
8. 에디터 전용 `USailFleetUIEditorLibrary`

Unreal Python으로 WBP를 만들기 전에 반드시 컴파일한다. Python 스크립트가
네이티브 부모 클래스와 에디터 브리지를 로드할 수 있어야 한다.

### 단계 C — Blender MCP 함선 제작

Blender MCP에서 큰 구조부터 작은 구조 순으로 만든다.

1. 용골과 loft 방식 선체
2. 늑골과 선체 두께 표현
3. 하부·상부 포갑판
4. 선수루와 선미루
5. 현측벽, 난간, 지주
6. 포구와 대포
7. 3개 돛대, 상부 돛대, 활대, 횡범, 삼각돛, 선수재
8. 방향타, 조타륜, 캡스턴, 해치, 계단, 삭구
9. 선미 갤러리와 함급별 장식

함급별로 별도 실루엣을 만든다.

- 1급 전열함: 가장 큰 2층 포갑판 선체, 넓은 돛 구성, 화려한 선미,
  금장, 왕실기, 페넌트, 랜턴, 선수상, 지휘 문양
- 2급 전열함: 축소된 2층 포갑판, 일부 포 위치 제거, 절제된 장식
- 중프리깃: 낮고 긴 1층 포갑판, 축소된 선미와 후미 돛 구성

`ASailShip`이 슬롯 이름 일부를 보고 머티리얼을 교체하므로 다음 이름을
보존한다.

```text
MAT_AgedCanvas
MAT_FlagshipPennant
MAT_OakHull
MAT_WeatheredDeck
MAT_FlagshipGold
MAT_CannonIron
MAT_DarkKeel
MAT_GalleryGlass
MAT_GunportVoid
```

세 함급의 원점, 스케일, 축을 통일한다. 임의의 Key/Fill/Rim 3점 조명과
카메라는 `.blend` 프리뷰에만 둔다. FBX는 선택한 함선 메시만 내보낸다.
Unreal에서는 함급당 하나의 Static Mesh로 결합하고, Material Slot은
가져오되 텍스처는 가져오지 않으며 Lightmap UV를 생성한다.

### 단계 D — ImageGen 래스터 제작

모든 결과에 넉넉한 안전 여백과 투명화하기 쉬운 배경을 둔다.

- 돛: 정사각 노화 캔버스, 파랑 또는 빨강 기반, 중앙의 큰 진영 문양,
  텍스트 없음, 넓은 카메라에서도 식별 가능
- 목재: 반복 가능한 낡은 선체 참나무와 밝은 갑판 판재, 평면 알베도에
  가까운 조명, 원근감 없음
- 바다 노멀: 반복 가능한 Tangent-space 청보라색 노멀, 교차하는 중·미세
  파도, 거품이나 그려진 하이라이트 없음
- 전투 FX: 총구·선체·물 피격 각각 별도 4×4 RGBA, 16칸 모두 중심·크기·
  여백 통일, 좌→우·상→하 순서
- 2D 함선: 진영별 4×2 RGBA, 순서
  `N, NE, E, SE / S, SW, W, NW`, 크기와 중심 통일
- 함장: 18세기 해군 유화풍 4×2 아틀라스, 7칸 사용, 파랑·빨강 제복 언어,
  일관된 흉상 구도, 텍스트 없음
- 로킷: Bronze/Silver/Gold 정사각 투명 이미지, 내부 타원 구멍 완전 동일
- 날짜: `0–9`, 점, 쉼표, 모든 축약 요일, 모든 월. 흰색 본문, 어두운
  아웃라인·아우터글로우, 넉넉한 여백. 월은 월 이름 전체가 한 이미지
- 풍향: 별도 투명 화살표. 나침반은 미사용 소스로 남을 수 있지만 현재
  HUD에는 화살표만 표시
- 타이틀: 정확한 3:1 투명 로고와 알파 형태 출항 버튼. 별도 사각 프레임
  추가 금지

### 단계 E — 오프라인 이미지 후처리

Pillow를 설치하고 다음을 실행한다.

```powershell
py -m pip install Pillow
py AgeOfSailFleet/Tools/slice_captain_portraits.py
py AgeOfSailFleet/Content/Python/generate_locket_portraits.py
py AgeOfSailFleet/Tools/slice_date_glyphs.py
py AgeOfSailFleet/Tools/slice_wind_icons.py
py AgeOfSailFleet/Tools/slice_ship_sprites.py
```

로킷 초상화 계약은 1254×1254 캔버스와 공통 구멍
`(366, 306, 885, 991)`이다. 함선 슬라이서는 각 방향 이미지를 늘이지 않고
512×512 투명 캔버스 중앙에 맞춘다. 날짜 슬라이서는 ImageGen 배경·격자를
제거하고 알파 경계로 자른 뒤 여백을 복구해 글자 잘림을 막는다.

## 7. Unreal 빌드와 에셋 생성

먼저 Unreal Editor를 닫는다. 열린 에디터는 Module DLL과 WBP 파일을
잠글 수 있다. `-ExecutePythonScript`에는 절대 경로를 사용한다. 상대
경로는 Engine Binary 폴더 기준으로 해석될 수 있다.

```powershell
$UE = 'C:\Program Files\Epic Games\UE_5.7'
$Repo = (Get-Location).Path
$Project = "$Repo\AgeOfSailFleet\AgeOfSailFleet.uproject"

& "$UE\Engine\Build\BatchFiles\Build.bat" `
  AgeOfSailFleetEditor Win64 Development `
  "-Project=$Project" -WaitMutex -NoHotReloadFromIDE

& "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $Project `
  "-ExecutePythonScript=$Repo\AgeOfSailFleet\Content\Python\import_game_assets.py" `
  -unattended -nop4 -nosplash -nullrhi

& "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $Project `
  "-ExecutePythonScript=$Repo\AgeOfSailFleet\Content\Python\create_fleet_map.py" `
  -unattended -nop4 -nosplash -nullrhi

& "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $Project `
  "-ExecutePythonScript=$Repo\AgeOfSailFleet\Content\Python\create_ui_assets.py" `
  -unattended -nop4 -nosplash -nullrhi
```

`import_game_assets.py`는 FBX와 래스터를 임포트하고 Texture/FX Material을
만들며 이중 바다 노멀 머티리얼을 구성한다. 바다 노멀은 sRGB Off,
Normal Compression, Wrap으로 설정하고 런타임 폰트 Wrapper도 만든다.

부분 반복 작업:

```powershell
# 전투 HUD만 재생성
-ExecutePythonScript=...\Content\Python\rebuild_fleet_hud.py

# 타이틀만 재생성
-ExecutePythonScript=...\Content\Python\rebuild_title_screen.py

# 로킷과 마스크 초상화만 재임포트
-ExecutePythonScript=...\Content\Python\import_locket_ui_assets.py
```

## 8. UMG 저작 불변 조건

`create_ui_assets.py`가 실제 `WidgetTree` 객체를 만들고 전체 계층을 연결한
뒤 필수 이름을 검사하고 WBP를 컴파일·저장한다. Designer에서 WBP를 열면
완전한 트리가 보여야 한다.

네이티브 코드가 해도 되는 일:

- `BindWidget` 필드 선언
- Delegate 연결·해제
- 텍스트, Brush, Opacity, Visibility, 선택 상태, 전환 상태 갱신

네이티브 코드가 하면 안 되는 일:

- `NativeConstruct`에서 트리 생성
- 불완전한 WBP를 보상하기 위한 런타임 Reparent
- 저작된 카드 슬롯을 런타임 동적 카드로 교체

UE 5.7 Python이 모든 `UWidgetBlueprint::WidgetTree` 작업을 안전하게
노출하지 않으므로 `USailFleetUIEditorLibrary`가 브리지 역할을 한다.
분리된 옛 위젯 변수와 GUID를 제거하고 현재 트리의 GUID만 다시 등록해
Compiler Ensure도 막는다.

`BindWidget` 이름을 바꿀 때는 C++와 Python 트리를 같은 변경에서 수정하고,
C++ 빌드 → Editor 재시작 → WBP 재생성 → 새 Commandlet에서 2차 로드를
수행한다. 최종 로그에 `LogBlueprint: Error`가 없어야 한다.

## 9. 런타임 아키텍처

| 계층 | 역할 |
|---|---|
| `ASailGameMode` | 타이틀·HUD·Director 생성, 선택된 3D/2D 전투 시작 |
| `AFleetBattleDirector` | 환경, 바람, 함대 Spawn, 전투 상태, 승패 |
| `ASailShip` | 함급, 아트 선택, 항해, AI, 포격, 피해, 침몰 |
| `AFleetPlayerController` | 선택, 대형 이동, 공격 명령, 카메라 모드 |
| `AFleetCameraPawn` | 함대 전략 카메라와 복원 가능한 자유 비행 |
| `ASailOceanActor` | 90,000uu 절차 파도 Grid와 파봉 색 |
| `AShipWakeActor` | 속도 기반 2열 거품 Ribbon |
| `ACannonballActor` | 탄도, 가시적 포탄·Trail, 선체·수면 피격 |
| `AFlipbookEffectActor` | 카메라를 보는 4×4 총구·선체·수면 애니메이션 |
| HUD·Title Widget | 바인딩과 표현만 담당 |

타이틀 전환 시간은 타이틀 페이드 0.45초, 암전 0.55초, 완전 암전 상태에서
전투 시작, 다시 밝아지는 시간 0.70초다.

## 10. 함대와 전투 수치

| 등급 | 함급 | 표기 포문 | HP | 시뮬레이션 일제사 |
|---|---|---:|---:|---:|
| 1 | First-rate Ship of the Line | 104 | 1650 | 포탄 8발 |
| 2 | Second-rate Ship of the Line | 90 | 1380 | 포탄 7발 |
| 3 | Heavy Frigate | 44 | 1120 | 포탄 6발 |

파랑 기함은 Admiral Elias Ward의 `HMS Sovereign Wind`, 빨강 기함은
Admiral Lucien Voss의 `RNS Imperieuse`다. 호위함 이름은
`ASailShip::ConfigureShip`에 정의돼 있다.

핵심 값:

- 기본 최고 속도 620, 가속 105, 선회 17도/초
- 포격 범위 4300, 선호 거리 3550
- 비상 분리 거리 2450, 회피 거리 3600
- 좌현·우현 재장전 각각 7.5초
- 포탄 속도 2350–2650 + 함선 속도, 상향 속도 300–430
- 포탄 중력 배율 0.42, 수명 8초
- 침몰 시작 후 14초에 숨기고 Tick 비활성화

AI는 목표의 접선 방향으로 접근하고 선호 포격 거리를 맞추며 주변 함선을
분리한다. 비상 거리 안에서는 강하게 후퇴하고 목표가 충분히 측면에 있을
때만 포격한다. 이 동작이 시작 직후 충각을 막는다.

## 11. 바다·FX·표현 계약

- 바다 Geometry는 61×61, Cell 1500uu, 전체 폭 90,000uu다.
- Geometry는 3개 Sine Wave를 합성하고 Material은 같은 Normal Texture를
  Tiling 18과 47, 반대 방향 Panner, Blend 0.42로 겹친다.
- Fog는 Density 0.0015, Max Opacity 0.55, Start Distance 10,000으로 약하게
  유지한다.
- 항적은 속도 55 이상일 때 0.18초마다 채집하고 7초간 유지한다.
- 플립북은 정확한 4×4이며 UV는 0.25 단위로 진행한다.
- 총구·선체·수면 이펙트는 함대 카메라에서도 보여야 한다. 호출부의 효과
  배율을 두 번 `/100` 하지 않는다.
- FX Material에는 Alpha가 필수이며 Translucent, Unlit, Two-sided다.
- 폰트는 런타임 `UFont` Wrapper가 필요하다. `FontFace`만 Slate에 넣으면
  LastResort `A`가 나타날 수 있다.

## 12. 실제 조작과 테스트 항목

- 좌클릭: 아군 함선 하나 선택
- 좌드래그: 박스 선택
- Shift + 클릭·드래그: 선택 추가
- 바다 우클릭: 대형 이동
- 적 우클릭: 공격
- `Ctrl+A`: 파랑 함대 전체 선택
- `WASD`: 전략 카메라 이동
- 휠: 전략 카메라 줌
- `C`: 자유 비행 토글
- 자유 비행: 마우스 시점, `WASD`, `Space`·왼쪽 `Ctrl`, 휠 속도

현재 실제 플레이에 연결되지 않은 Q/E 수동 포격·수동 항해 Mapping은
조작 안내에 넣지 않는다. Possess된 Pawn은 함대 카메라이며 함선 포격은
현재 AI가 수행한다.

## 13. 확인된 구현 커밋

Age of Sail 프로젝트는 다음 순서로 구현됐다.

1. `395f2ff` — UE 5.7 골격과 네이티브 계약
2. `66a0ea0` — Blender·ImageGen 에셋, 핵심 함대전, UI, 바다, FX
3. `34f0ff5` — 바다 노멀·머티리얼과 UI 수정
4. `919c08d` — 2D 모드, 로킷, 넓은 배치, 카메라·Fog 개선
5. `8e83911` — 거리 유지 측면 포격 AI와 침몰
6. `ff8593e` — 날짜·풍향 HUD 가시성 수정
7. `38bcdf5` — 투명한 타이틀·HUD 표현 정리
8. `c207795` — 자유 비행과 3D/2D 라디오 버튼

대형 통합 커밋 내부의 실제 작업 순서는 Git만으로 복원할 수 없다. 6~7절은
최종 소스 의존성으로 추론한 가장 안전한 재현 순서다.

## 14. 검증

Headless Smoke Test:

```powershell
& "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $Project `
  /Game/Maps/FleetOcean -game -AutoStartBattle -FleetSmokeTest `
  -unattended -nop4 -nosplash -nullrhi -nosound -log
```

성공 로그:

```text
Fleet smoke test passed ships_blue=3 ships_red=4 wind=0.80
```

시각 검증 항목:

- WBP Designer에 타이틀 전체 트리와 HUD 카드 8개가 실제로 보인다.
- 3D·2D 라디오 버튼은 하나만 선택되며 기본값은 3D다.
- 로고는 3:1 비율이고 버튼·패널에 불필요한 사각형이 없다.
- 세 함급이 서로 다른 메시와 Material Slot을 사용한다.
- 돛만 보고 파랑·빨강 진영을 구분할 수 있다.
- 함선이 측면 거리를 유지하고 포격 효과가 보이며 피해 후 침몰한다.
- 빗나가면 수면 피격, 명중하면 선체 피격이 보이고 포탄·Trail도 넓은
  카메라에서 읽힌다.
- 날짜 간격이 좁고 어두운 배경이 없으며 풍향 화살표만 보인다.
- 자유 비행 종료 시 `C`를 누르기 전 전략 카메라 상태가 정확히 복원된다.

## 15. 흔한 실패 원인

- **DLL·WBP 저장 실패:** Build나 Commandlet 실행 전에 모든 Unreal Editor
  Process를 닫는다.
- **Python 파일을 못 찾음:** 절대 `-ExecutePythonScript` 경로를 사용한다.
- **WBP Binding 오류:** Native를 먼저 빌드하고 Script·C++ 이름을 맞춘 뒤
  재생성하고 새 Process에서 다시 검사한다.
- **위젯이 좌상단에 겹침:** Canvas Slot Editor Property 쓰기 대신 Setter
  API를 사용한다.
- **글자가 잘림:** Alpha 경계로 Crop한 뒤 안전 Padding을 복원한다.
- **초상화·프레임 사이 틈:** 공통 Aperture, 초상화 Overscan, 프레임 상단
  렌더 순서를 사용한다.
- **바다 조명이 이상함:** Normal을 sRGB Off, Normal Compression, Wrap으로
  임포트한다.
- **함선 텍스처가 안 붙음:** Blender Material Slot 이름 계약을 복원한다.
- **이펙트는 Spawn되지만 안 보임:** World Size 계산, Material Alpha,
  Bounds, Camera-facing 회전, 함대 카메라 기준 스케일을 확인한다.

