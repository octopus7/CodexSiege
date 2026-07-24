# Age of Sail: Fleet Command — Reproduction Guide

## 1. Purpose and provenance

This document explains how another developer can recreate the `AgeOfSailFleet`
prototype from an empty Unreal project. It describes the production order,
source contracts, automation, architecture, validation, and the mistakes that
were found during iteration.

According to the project owner's production record:

- The project source, configuration, automation, and integration were authored
  through Codex conversations rather than hand-written outside Codex.
- Blender was the only external DCC application, and Codex controlled it through
  Blender MCP.
- Raster concepts, textures, atlases, portraits, UI art, and flipbooks were
  produced through ImageGen and then processed/imported by Codex-authored tools.
- The work started with the Sol model at `XHigh` reasoning and continued at
  `High` reasoning.

Those four points are the owner's process record, not facts recoverable from Git
metadata. Git does prove the implementation and asset milestones described in
section 13.

## 2. Reproducibility boundary

The repository contains editable Blender sources, exported FBX files, raw and
processed PNG files, C++ source, Unreal Python automation, and checked-in Unreal
assets. It does **not** contain the complete Blender MCP command transcript,
verbatim ImageGen prompts, seeds, or hidden model state.

Therefore:

- A functional and visually equivalent recreation is practical.
- Rebuilding the checked-in project from its source assets is practical.
- Pixel-identical ImageGen regeneration or an identical Blender operation
  history is not guaranteed.
- `*_Source.png` files and `.blend` files are audit/edit sources; processed PNG,
  FBX, `.uasset`, and `.umap` files are pipeline outputs.

The prompt templates below are reconstructed production specifications, not
archived verbatim prompts.

## 3. Final target

The recreated prototype should provide:

- A title screen over the ocean, a 3:1 cinematic logo, an image-shaped departure
  button, round mutually exclusive `3D`/`2D` radio controls, and a
  title-fade → black → battle-fade transition.
- Three blue ships against four red ships, initially separated by 32,000 Unreal
  units between flagships.
- Three distinct rates: first-rate ship of the line, second-rate ship of the
  line, and heavy frigate.
- Click, box, additive, and select-all fleet selection; right-click formation
  movement and right-click attack orders.
- Wind-dependent speed, broadside positioning, collision avoidance, cannon
  ballistics, damage, destruction effects, and sinking.
- A procedural ocean, layered wave normal, visible wakes, cannon muzzle flashes,
  hull impacts, water splashes, enlarged cannonballs, and ballistic trails.
- A complete 3D Blender-authored presentation plus a faction-colored
  camera-relative eight-direction 2D sprite mode.
- A battle HUD with oval captain portraits under interchangeable
  bronze/silver/gold locket frames, captain and ship names, image-only date
  glyphs, and a wind arrow.
- `C` toggling between the strategy camera and free flight.

## 4. Required environment

- Windows 10/11
- Unreal Engine 5.7
- Visual Studio 2022 with Desktop development with C++, a Windows SDK, and the
  Unreal-compatible MSVC toolchain
- Blender capable of opening the checked-in `.blend` files
- Codex with Blender MCP when recreating the original modeling workflow
- ImageGen access for regenerating raster art
- Python 3.11+ and Pillow for the offline atlas/mask tools

The Unreal project enables:

- `ProceduralMeshComponent`
- `PythonScriptPlugin`
- `EditorScriptingUtilities`

The module uses `Core`, `CoreUObject`, `Engine`, `InputCore`,
`ProceduralMeshComponent`, `UMG`, `Slate`, and `SlateCore`, plus editor-only
`UMGEditor` and `UnrealEd`.

## 5. Source-of-truth layout

```text
AgeOfSailFleet/
  Art/
    Source/                 Editable Blender master scenes
    Preview/                Preview-only three-point-lit renders
  Config/                   Maps, game mode, renderer, and input settings
  Content/
    Raw/                    FBX and raw/processed raster source files
    Python/                 Unreal Editor import and authoring automation
    Art/                    Imported meshes, materials, textures, sprites, FX
    UI/                     Imported UI textures, font, and authored WBPs
    Maps/FleetOcean.umap    Minimal runtime map
  Source/AgeOfSailFleet/    Native runtime and editor bridge
  Tools/                    Pillow atlas slicing and cleanup tools
```

Important source assets:

- `Art/Source/AgeOfSailWarship.blend`
- `Art/Source/AgeOfSailFleetVariants.blend`
- `Content/Raw/AgeOfSailWarship_FirstRate.fbx`
- `Content/Raw/AgeOfSailWarship_SecondRate.fbx`
- `Content/Raw/AgeOfSailWarship_Frigate.fbx`
- `Content/Raw/AgeOfSailWarship.fbx` as a safe fallback

## 6. Recommended implementation order

### Phase A — Freeze the design contract

Before writing code, state the invariants:

1. Ships fight with broadsides and maintain range; they do not use ramming as
   their normal tactic.
2. The three ship rates require different meshes, collision sizes, statistics,
   silhouettes, and presentation scales.
3. Blue and red fleets must be identifiable by sail color and insignia.
4. A flagship receives more elaborate stern, gold, pennant, gallery, lantern,
   crown/figurehead, and command decoration.
5. Wind affects movement and is visible in the HUD.
6. Sufficient damage starts a visible sinking sequence.
7. The full UMG hierarchy exists inside the WBP asset. Native constructors and
   `NativeConstruct` never assemble the widget tree.
8. Blender preview cameras and lights are never exported to Unreal.

### Phase B — Create the UE C++ scaffold

Create an Unreal Engine 5.7 C++ game named `AgeOfSailFleet`. Add the plugins and
module dependencies listed above. Configure:

- default/editor map: `/Game/Maps/FleetOcean`
- global game mode: `/Script/AgeOfSailFleet.SailGameMode`
- DX12, Lumen GI/reflections, and mesh distance fields

Implement the native layers in this dependency order:

1. `ACannonballActor`
2. `ASailShip`
3. `AFleetBattleDirector` and `ASailOceanActor`
4. `AShipWakeActor` and `AFlipbookEffectActor`
5. `AFleetCameraPawn` and `AFleetPlayerController`
6. `USailFleetHUDWidget` and `USailTitleScreenWidget`
7. `ASailGameMode`
8. editor-only `USailFleetUIEditorLibrary`

Compile before asking Unreal Python to create Widget Blueprints. The Python
scripts need the native parent classes and editor bridge to be loadable.

### Phase C — Build the ships through Blender MCP

Use Blender MCP to build in large-to-small dependency order:

1. keel and lofted hull
2. ribs and hull thickness cues
3. lower/upper gun decks
4. forecastle and quarterdeck
5. bulwarks, rails, and stanchions
6. gunports and cannons
7. three masts, topmasts, yards, square sails, jibs, and bowsprit
8. rudder, wheel, capstan, hatches, stairs, and rigging
9. stern gallery and class-specific decoration

Produce separate first-rate, second-rate, and frigate silhouettes:

- First-rate: largest two-deck hull, broad sail plan, richest stern, gold trim,
  royal standard, pennants, lanterns, figurehead, and command emblems.
- Second-rate: smaller two-deck hull with fewer represented positions and
  restrained ornament.
- Heavy frigate: lower, longer one-gun-deck hull with reduced stern and mizzen.

Preserve these material-slot names because `ASailShip` replaces materials by
slot-name substring:

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

Keep origin, scale, and axes identical across all variants. Add an arbitrary
key/fill/rim lighting rig and camera only for the `.blend` preview. Export only
the selected ship meshes. The Unreal import combines each rate into one static
mesh, imports material slots, does not import textures, and generates lightmap
UVs.

### Phase D — Generate raster assets with ImageGen

Use safe margins and produce transparent or cleanup-friendly backgrounds.

- Sails: square aged canvas, blue or red base, large centered faction insignia,
  no text, readable at fleet-camera distance.
- Timber: square tileable weathered hull oak and lighter deck planks, flat
  albedo-like lighting, no perspective.
- Ocean normal: seamless tangent-space blue/purple normal, medium and fine
  crossing waves, no foam or painted highlights.
- Combat FX: three separate 4×4 RGBA sheets for cannon muzzle, hull hit, and
  water impact. Sixteen centered cells, left-to-right then top-to-bottom.
- Directional ships: faction-specific 4×2 RGBA atlases ordered
  `N, NE, E, SE / S, SW, W, NW`, with identical scale and center.
- Captains: consistent 18th-century naval oil portraits in a 4×2 atlas, seven
  occupied cells, blue/red uniform language, no text.
- Locket frames: bronze/silver/gold square transparent images with the exact
  same oval aperture.
- Date glyphs: digits `0–9`, dot, comma, all abbreviated weekdays, and all
  month names. White face, dark outline/outer glow, generous safe margins.
  Each month is one glyph image.
- Wind: a separate transparent arrow; a compass may remain as an unused source
  asset, but the current HUD displays only the arrow.
- Title: a transparent exact 3:1 logo and an alpha-shaped departure button,
  without an additional rectangular UI frame.

### Phase E — Run offline raster processing

Install Pillow, then run:

```powershell
py -m pip install Pillow
py AgeOfSailFleet/Tools/slice_captain_portraits.py
py AgeOfSailFleet/Content/Python/generate_locket_portraits.py
py AgeOfSailFleet/Tools/slice_date_glyphs.py
py AgeOfSailFleet/Tools/slice_wind_icons.py
py AgeOfSailFleet/Tools/slice_ship_sprites.py
```

The locket portrait contract is a 1254×1254 canvas with shared aperture
`(366, 306, 885, 991)`. The ship slicer fits each direction without stretching
onto a centered 512×512 transparent canvas. The date slicer removes the
ImageGen background/grid, crops alpha bounds, and restores padding to prevent
clipped letters.

## 7. Build and generate Unreal assets

Close Unreal Editor first. An open editor can lock the module DLL and WBP files.
Use absolute Python script paths; relative `-ExecutePythonScript` paths can
resolve from the engine binary directory.

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

`import_game_assets.py` imports the FBX files and raster assets, creates texture
and FX materials, builds the two-layer ocean normal material, configures the
normal map as non-sRGB/normal compression/wrap, and creates the runtime font
wrapper.

For focused iteration:

```powershell
# Rebuild only the battle HUD
-ExecutePythonScript=...\Content\Python\rebuild_fleet_hud.py

# Rebuild only the title screen
-ExecutePythonScript=...\Content\Python\rebuild_title_screen.py

# Reimport only locket frames and masked portraits
-ExecutePythonScript=...\Content\Python\import_locket_ui_assets.py
```

## 8. UMG authoring invariant

`create_ui_assets.py` creates real `WidgetTree` objects, attaches the complete
hierarchy, validates required names, compiles the Widget Blueprint, and saves it.
Opening either WBP in Designer must show an intact tree.

Native code may:

- declare `BindWidget` fields
- bind/unbind delegates
- update text, brushes, opacity, visibility, selection state, and animation state

Native code must not:

- construct the tree in `NativeConstruct`
- reparent widgets at runtime to compensate for an incomplete WBP
- replace authored card slots with dynamically created runtime cards

`USailFleetUIEditorLibrary` exists because UE 5.7 Python does not expose all
`UWidgetBlueprint::WidgetTree` operations safely. It also clears detached widget
variables and rebuilds the GUID map from live widgets to avoid stale compiler
ensures.

When a `BindWidget` name changes, change C++ and the Python-authored tree
together, rebuild C++, restart the editor, regenerate the WBP, and run a second
clean commandlet load. The final log must contain no `LogBlueprint: Error`.

## 9. Runtime architecture

| Layer | Responsibility |
|---|---|
| `ASailGameMode` | Creates title/HUD/director and starts the selected 3D/2D battle |
| `AFleetBattleDirector` | Environment, wind, fleet spawning, battle state, victory |
| `ASailShip` | Rate data, art selection, sailing, AI, broadsides, damage, sinking |
| `AFleetPlayerController` | Selection, formation move, attack orders, camera mode |
| `AFleetCameraPawn` | Fleet strategy camera and restorable free flight |
| `ASailOceanActor` | 90,000uu procedural wave grid and crest coloring |
| `AShipWakeActor` | Speed-dependent paired foam ribbons |
| `ACannonballActor` | Ballistic projectile, visible ball/trail, ship/water impact |
| `AFlipbookEffectActor` | Camera-facing 4×4 muzzle/hull/water animation |
| HUD/title widgets | Binding and presentation only |

The title sequence uses 0.45 s title fade, 0.55 s fade to black, battle start at
black, and 0.70 s fade back.

## 10. Fleet and combat values

| Rate | Class | Guns metadata | HP | Simulated broadside |
|---|---|---:|---:|---:|
| 1 | First-rate Ship of the Line | 104 | 1650 | 8 projectiles |
| 2 | Second-rate Ship of the Line | 90 | 1380 | 7 projectiles |
| 3 | Heavy Frigate | 44 | 1120 | 6 projectiles |

Blue flagship: Admiral Elias Ward, `HMS Sovereign Wind`.
Red flagship: Admiral Lucien Voss, `RNS Imperieuse`.
Escort names are defined in `ASailShip::ConfigureShip`.

Core tuning:

- base maximum speed 620, acceleration 105, turn rate 17°/s
- broadside range 4300, preferred range 3550
- emergency separation 2450, avoidance 3600
- port/starboard reload 7.5 s
- ballistic speed 2350–2650 plus ship velocity, upward 300–430
- projectile gravity scale 0.42 and lifetime 8 s
- sinking lasts 14 s before hiding/disabling the ship

AI approaches on a tangent, corrects toward broadside range, separates from
neighbors, retreats inside emergency range, and fires only when the target is
properly lateral. This is the behavior that prevents opening rams.

## 11. Ocean, effects, and presentation contracts

- Ocean geometry: 61×61 grid, 1500uu cells, 90,000uu total width.
- Geometry uses three sine layers; the material adds the same normal texture at
  tiling 18 and 47 with opposing panners and a 0.42 blend.
- Fog remains light: density 0.0015, max opacity 0.55, start distance 10,000.
- Wake samples every 0.18 s above speed 55 and retains points for 7 s.
- Flipbooks are exactly 4×4; runtime UVs advance in 0.25 increments.
- Cannon muzzle, hull impact, and water impact must be large enough for the fleet
  camera. Do not divide a caller-supplied effect multiplier by 100 twice.
- Material alpha is required; FX materials are translucent, unlit, and two-sided.
- Font import requires a runtime `UFont` wrapper. Passing only a `FontFace` to
  Slate can produce LastResort `A` placeholders.

## 12. Controls to document and test

- Left click: select one friendly ship
- Left drag: box select
- Shift + click/drag: additive selection
- Right click sea: move in formation
- Right click enemy: attack
- `Ctrl+A`: select all blue ships
- `WASD`: strategy pan
- Mouse wheel: strategy zoom
- `C`: toggle free flight
- Free flight: mouse look, `WASD`, `Space`/left `Ctrl`, wheel speed

Do not advertise the dormant Q/E/manual sailing mappings as active gameplay.
The possessed pawn is the fleet camera, and ship firing is currently AI-driven.

## 13. Verified implementation milestones

The Age of Sail project was built through these commits:

1. `395f2ff` — initial UE 5.7 scaffold and native contracts
2. `66a0ea0` — Blender/ImageGen assets, core fleet simulation, UI, ocean, FX
3. `34f0ff5` — ocean normal/material and UI fixes
4. `919c08d` — 2D mode, lockets, wider deployment, camera/fog polish
5. `8e83911` — broadside spacing AI and sinking
6. `ff8593e` — visible date and wind HUD
7. `38bcdf5` — transparent title/HUD presentation cleanup
8. `c207795` — free-flight camera and 3D/2D radio controls

The large integration commit does not reveal its internal operation order.
Sections 6–7 present the safest dependency order inferred from the final source.

## 14. Validation

Run the headless smoke test:

```powershell
& "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $Project `
  /Game/Maps/FleetOcean -game -AutoStartBattle -FleetSmokeTest `
  -unattended -nop4 -nosplash -nullrhi -nosound -log
```

Success requires:

```text
Fleet smoke test passed ships_blue=3 ships_red=4 wind=0.80
```

Also verify visually:

- WBP Designer shows complete title and eight-card HUD trees.
- 3D and 2D radio choices are mutually exclusive and default to 3D.
- The logo keeps 3:1 aspect and buttons/panels have no unwanted rectangles.
- All three rates use distinct meshes and material slots.
- Sails clearly distinguish blue/red factions.
- Ships maintain broadside distance, fire visible effects, and sink.
- Misses create water impacts; hits create hull impacts; cannonballs and trails
  remain readable from the fleet camera.
- The date has tight spacing and no dark background; only the wind arrow shows.
- `C` restores the exact strategy camera state after free flight.

## 15. Common failure modes

- **DLL or WBP cannot save:** close every Unreal Editor process before build or
  commandlet generation.
- **Python file not found:** use an absolute `-ExecutePythonScript` path.
- **WBP binding errors:** build native code first, keep script/C++ names equal,
  regenerate, then verify in a fresh commandlet process.
- **Widgets overlap at the top-left:** use Canvas slot setter APIs rather than
  relying on editor-property writes.
- **Letters are clipped:** crop by alpha bounds and restore safe padding.
- **Portrait frame gap:** use the shared aperture, overscan the portrait, and
  render the frame above it.
- **Wrong ocean shading:** import the normal with sRGB off, normal compression,
  and wrap addressing.
- **Ship textures do not replace:** restore the expected Blender material-slot
  names.
- **Effects technically spawn but are invisible:** check world-size math,
  material alpha, bounds, camera-facing rotation, and fleet-camera scale.

