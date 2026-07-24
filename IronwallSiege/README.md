# Ironwall Siege — Unreal Engine 5.7 Prototype

영문 UI를 기반으로 한 중세 공성전 C++ 프로토타입입니다. 별도의 게임용 `.uasset`이 없어도
성벽, 성문, 탑, 투석기, 공성추, 병사, 지형을 런타임 절차형 메시로 생성합니다.

## 빠른 실행

1. Unreal Engine 5.7과 C++ 빌드 도구를 설치합니다.
2. `IronwallSiege.uproject`를 엽니다.
3. 모듈 빌드 요청을 수락합니다.
4. 에디터 모듈이 `/Game/Data/DA_PrototypeSiege`와
   `/Game/Data/DA_BlenderProduction`을 자동 생성할 때까지 기다립니다.
5. Play를 누릅니다.

기본 맵은 엔진의 `/Engine/Maps/Entry`를 사용하며 `ASiegeWorldDirector`가 데모 공성 장면을
코드로 구성합니다. 타이틀 화면의 **BEGIN SIEGE**를 누르면 자유 카메라로 전환됩니다.

## 조작

- `W/A/S/D`: 카메라 이동
- `Q/E`: 하강/상승
- 마우스: 시점 회전
- `Esc`: 타이틀 메뉴 열기/닫기

## 리소스 세트

- **Prototype Geometry**: 항상 절차형 C++ 메시를 사용합니다.
- **Blender Production**: 정해진 경로에 Static Mesh가 있으면 자동으로 사용하고,
  아직 없는 슬롯만 절차형 메시로 폴백합니다.

두 세트는 `USiegeGameInstance::AvailableResourceSets`에 연결되어 있고 선택 상태는
`USiegeUserSettings`에 저장됩니다. 옵션 메뉴에서 런타임 전환할 수 있습니다.

## Blender 교체 경로

| 슬롯 | Unreal 경로 |
| --- | --- |
| Ground | `/Game/Art/Blender/SM_Ground_SiegeField` |
| Wall | `/Game/Art/Blender/SM_Fortress_Wall_A` |
| Gate | `/Game/Art/Blender/SM_Fortress_Gatehouse_A` |
| Tower | `/Game/Art/Blender/SM_Fortress_Tower_A` |
| Trebuchet | `/Game/Art/Blender/SM_Siege_Trebuchet_A` |
| Battering ram | `/Game/Art/Blender/SM_Siege_BatteringRam_A` |
| Infantry | `/Game/Art/Blender/SM_Unit_Infantry_A` |

FBX를 위 이름 그대로 `/Game/Art/Blender`에 가져오면 코드 변경이나 DA 재연결 없이 교체됩니다.
세부 모델링 규격과 MCP 실행 순서는 `Docs/BLENDER_MCP_EXECUTION_GUIDE.md`를 참고하세요.

## 중요한 제약

이 패키지는 Unreal Editor가 없는 환경에서 작성되어 실제 UE 컴파일·Cook·패키징은 실행하지
못했습니다. 소스, 설정, 생성 이미지, 자동 DA 부트스트랩은 정적 검사되었으며 최종 검증은
UE 5.7이 설치된 장비에서 진행해야 합니다.
