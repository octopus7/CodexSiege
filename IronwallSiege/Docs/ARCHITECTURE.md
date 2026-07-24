# Architecture

## Runtime flow

1. `USiegeGameInstance` owns soft references to both resource-set Data Assets.
2. The editor module creates the two Data Assets on first editor startup.
3. `ASiegeWorldDirector` spawns modular `ASiegeAssetProxyActor` instances.
4. Each proxy asks the active resource set for its slot-specific Static Mesh.
5. If the mesh resolves, the proxy enables `UStaticMeshComponent`.
6. If it does not resolve, the proxy builds the equivalent geometry in
   `UProceduralMeshComponent`.
7. The options combo changes the active set, saves the selection, and broadcasts a refresh.

## Replacement boundary

Gameplay placement and visual content are deliberately separated. World assembly only knows
`ESiegeAssetSlot`; it does not know asset paths. The resource Data Asset owns those paths.
This lets a Blender mesh replace a placeholder without changing spawning logic, transforms,
options UI, or save data.

## Native fallback

The game instance also constructs transient native definitions for both sets. The title menu and
procedural demo therefore still work before the editor has generated the `.uasset` Data Assets.
The production fallback already contains the final soft object paths.

## Title screen

`USiegeTitleWidget` is built entirely in C++ and reads
`Content/Raw/UI/TitleBackdrop.png` at runtime. Packaging stages `Content/Raw` as Non-UFS so
the image remains replaceable without a Widget Blueprint.
