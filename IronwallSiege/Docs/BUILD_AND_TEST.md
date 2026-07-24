# UE 5.7 Build and Test Checklist

## Prerequisites

- Unreal Engine 5.7
- Visual Studio 2022 with Desktop development with C++ and Game development with C++
  on Windows, or the supported Clang toolchain on macOS/Linux

## First open

1. Right-click `IronwallSiege.uproject` and generate project files if your platform requires it.
2. Build the `IronwallSiegeEditor` Development Editor target.
3. Open the project and verify that `Content/Data` contains both DA assets.
4. Play In Editor.

## Acceptance checks

- Title image fills the viewport and all visible text is English.
- **BEGIN SIEGE** closes the menu.
- `W/A/S/D`, `Q/E`, and mouse camera controls work.
- The generated fortress includes four walls, a gatehouse, two towers, one trebuchet,
  one battering ram, and infantry ranks.
- Options lists both resource sets.
- Switching sets refreshes every placed proxy.
- Selecting Blender Production before importing models preserves the procedural scene.
- Importing any one correctly named mesh replaces only its matching slot.
- The selected resource set persists after restarting the game.

## Packaging

Use **Platforms → Package Project**. Confirm that `Raw/UI/TitleBackdrop.png` appears in the
staged Non-UFS `Content/Raw/UI` directory. If your deployment policy requires everything inside
Pak/IoStore, import the PNG as a Texture2D and swap the loader for a soft asset reference.
