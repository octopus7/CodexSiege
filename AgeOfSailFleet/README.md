# Age of Sail: Fleet Command

A standalone Unreal Engine 5.7 C++ fleet-battle prototype. Three distinct
Blender-authored ship rates provide their own hull, gun-deck, cannon, mast,
sail, rail, and deck-detail silhouettes. ImageGen-authored sails, timber,
captains, title art, date glyphs, wind instruments, and combat flipbooks are
imported as project assets; the procedural ship remains only as a safe fallback.

## Controls

- Left click: select one friendly ship
- Left drag: box-select multiple friendly ships
- Shift + left click/drag: add ships to the selection
- Right click sea: move selected ships in formation
- Right click enemy: attack with selected ships
- `Ctrl + A`: select the entire blue fleet
- `W / A / S / D`: move the fleet camera
- Mouse wheel: zoom

Open `AgeOfSailFleet.uproject`, press Play, then choose **DEPART WITH THE FLEET**
on the title screen. You command three blue-and-gold ships against a four-ship
red fleet.

The title screen's round **GRAPHIC MODE** radio controls select either the
authored **3D** ships or the ImageGen-painted **2D** presentation. In 2D mode, each ship uses
eight camera-relative directional sprites while retaining the same fleet AI,
movement, collision, combat, and sinking simulation.
Automated runs can force the same mode with `-Graphics2D`.

The runtime includes wind-aware sailing, formation commands, autonomous
broadside combat, ship-rate-specific guns and health, procedural ocean waves,
foam wakes, sinking, and flipbook cannon/hull/water effects.

## Authored assets

- `Art/Source/AgeOfSailFleetVariants.blend`: editable ship-rate master scene
- `Art/Source/AgeOfSailFleetVariants_GalleonRefined.blend`: production-detail
  pass with curved hull wales, beakheads, figureheads, layered stern galleries,
  lifeboats, anchors, lanterns, and dense ratline rigging
- `Art/Reference/Galleon_Design_Concept.png`: ImageGen design reference used for
  the refined galleon silhouette and detail language
- `Art/Preview/FleetClasses.png`: preview-only three-point-lit fleet lineup
- `Content/Raw/Textures`: ImageGen sail and timber source textures
- `Content/Raw/UI`: ImageGen captain, title, date, and wind source art
- `Content/Raw/FX`: ImageGen 4x4 combat flipbook sheets
- `Content/Raw/Sprites/Ships`: ImageGen 8-direction blue/red ship sprite atlases

Blender preview lights and cameras are not exported to Unreal.

## Reproduction documentation

- [English reproduction guide](Docs/REPRODUCTION_GUIDE_EN.md)
- [한국어 재현 가이드](Docs/REPRODUCTION_GUIDE_KO.md)
