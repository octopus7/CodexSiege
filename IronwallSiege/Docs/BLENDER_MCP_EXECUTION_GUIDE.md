# Blender MCP 실행 지침

이 문서는 작업 장비에서
[ahujasid/blender-mcp](https://github.com/ahujasid/blender-mcp)를 연결하고,
이 프로젝트용 모델을 생성·검증·내보내는 순서입니다. 저장소 README를 2026-07-24에
확인하여 작성했습니다.

## 1. 설치 전 확인

- Blender 3.0 이상
- Python 3.10 이상
- `uv`와 `uvx`
- Blender MCP 저장소의 최신 `addon.py`

공식 저장소는 Python 3.11 고정을 권장합니다. GUI에서 실행한 MCP 클라이언트는 터미널의
PATH를 상속하지 않을 수 있으므로 `uvx`를 찾지 못하면 Windows는 `where uvx`,
macOS/Linux는 `which uvx`로 절대 경로를 확인하세요.

## 2. Blender 애드온 설치

1. 공식 저장소에서 최신 `addon.py`를 받습니다.
2. Blender에서 **Edit → Preferences → Add-ons → Install…**을 선택합니다.
3. `addon.py`를 설치하고 **Interface: Blender MCP**를 활성화합니다.
4. 새 `.blend` 파일을 열고 3D View에서 `N`을 눌러 **BlenderMCP** 탭을 엽니다.
5. 이번 작업에서는 Poly Haven, Sketchfab, Hyper3D, Hunyuan3D 옵션을 끕니다.

## 3. MCP 클라이언트 설정

권장 설정:

```json
{
  "mcpServers": {
    "blender": {
      "command": "uvx",
      "args": ["--python", "3.11", "blender-mcp"],
      "env": {
        "UV_PYTHON_PREFERENCE": "only-managed",
        "BLENDER_HOST": "localhost",
        "BLENDER_PORT": "9876",
        "DISABLE_TELEMETRY": "true"
      }
    }
  }
}
```

Windows GUI 클라이언트에서 `spawn uvx ENOENT`가 나면 다음처럼 바꿉니다.

```json
{
  "mcpServers": {
    "blender": {
      "command": "cmd",
      "args": ["/c", "uvx", "--python", "3.11", "blender-mcp"],
      "env": {
        "UV_PYTHON_PREFERENCE": "only-managed",
        "BLENDER_HOST": "localhost",
        "BLENDER_PORT": "9876",
        "DISABLE_TELEMETRY": "true"
      }
    }
  }
}
```

`uvx` 절대 경로를 `command`로 직접 지정해도 됩니다. 설정을 바꾼 뒤 MCP 클라이언트를
완전히 종료하고 다시 실행합니다. Blender MCP 서버는 한 클라이언트에서만 실행하세요.
별도 터미널에서 `uvx blender-mcp`를 동시에 실행하지 않습니다.

## 4. 연결

1. MCP 클라이언트를 시작합니다.
2. Blender의 **BlenderMCP** 패널에서 **Connect to Claude**를 누릅니다. 버튼 이름은
   다른 MCP 클라이언트를 사용해도 동일할 수 있습니다.
3. MCP 도구로 `get_scene_info`를 한 번 실행합니다.
4. 연결 실패 시 Blender 애드온 서버, 포트 `9876`, 단일 MCP 서버 실행 여부를 확인하고
   Blender와 MCP 클라이언트를 완전히 재시작합니다.

## 5. 작업 큐 실행

`BlenderMCP/prompts`의 파일을 번호 순으로 MCP 대화에 붙여 넣습니다.

1. `00_SESSION_SETUP.md`
2. `01_FORTRESS_KIT.md`
3. `02_SIEGE_ENGINES.md`
4. `03_INFANTRY_AND_GROUND.md`
5. `04_VALIDATE_AND_EXPORT.md`

각 단계가 끝날 때 `.blend`를 저장하고 뷰포트 스크린샷으로 확인하세요. 복잡한 모델을 한
프롬프트에서 전부 만들지 말고, 프롬프트 안의 자산 단위로 작업을 나눕니다.

## 6. 검증 스크립트

`BlenderMCP/scripts/ue_asset_pipeline.py`는 다음만 수행합니다.

- 필수 루트 이름, 적용되지 않은 Scale, UV0, 재질 슬롯, 크기, UCX 충돌체 검사
- 사용자가 승인한 디렉터리에만 FBX 내보내기

Blender MCP의 `execute_blender_code`는 Blender 프로세스 권한으로 임의 Python 코드를
실행할 수 있습니다. 스크립트를 먼저 직접 검토하고 `.blend`를 저장한 뒤 실행하세요.
알 수 없는 프롬프트가 제안하는 네트워크, 셸, 패키지 설치, 자격 증명 접근 코드는 허용하지
마세요.

MCP에 보낼 검증 요청 예:

```text
Review BlenderMCP/scripts/ue_asset_pipeline.py, then load it in Blender and call
validate_scene(). Do not call export_assets(), access the network, run a subprocess,
install anything, or write any file.
```

오류가 모두 해결된 뒤 내보내기 디렉터리를 명시합니다.

```text
Call export_assets(r"D:\IronwallSiege\BlenderExports"). Write only to that directory.
```

## 7. Unreal Engine 가져오기

1. UE 5.7 Content Browser에서 `/Game/Art/Blender` 폴더를 만듭니다.
2. 내보낸 FBX 7개를 가져옵니다.
3. Static Mesh 이름이 다음과 정확히 일치하는지 확인합니다.

   - `SM_Ground_SiegeField`
   - `SM_Fortress_Wall_A`
   - `SM_Fortress_Gatehouse_A`
   - `SM_Fortress_Tower_A`
   - `SM_Siege_Trebuchet_A`
   - `SM_Siege_BatteringRam_A`
   - `SM_Unit_Infantry_A`

4. **Combine Meshes**는 루트별 단일 Static Mesh가 필요할 때만 켭니다. 향후 애니메이션
   대상인 문짝·투석기 팔·바퀴·무기는 Blender 원본에서 분리 상태를 유지합니다.
5. 옵션에서 **Blender Production**을 선택합니다. 정확한 경로에 있는 슬롯부터 자동으로
   교체되고 나머지는 절차형 폴백을 유지합니다.

## 8. 문제 해결

- `spawn uvx ENOENT`: `uvx` 절대 경로를 사용하고 클라이언트를 완전히 재시작합니다.
- Python/Conda 충돌: `--python 3.11`과 `UV_PYTHON_PREFERENCE=only-managed`를 유지합니다.
- 이전 설치 실패가 반복됨: 공식 README 지침대로 `uv cache clean blender-mcp` 후
  `uvx --refresh blender-mcp`를 한 번 실행해 캐시를 갱신합니다.
- 첫 명령만 실패: 애드온 연결 버튼과 단일 서버 상태를 재확인한 뒤 한 번 더
  `get_scene_info`를 실행합니다.
- 타임아웃: 한 자산 또는 한 수정으로 요청을 더 작게 나눕니다.
