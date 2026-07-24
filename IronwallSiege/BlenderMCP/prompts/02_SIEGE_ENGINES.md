# Paste-ready MCP prompt — siege engines

Read `BlenderMCP/manifests/siege_asset_manifest.json` and model only
`SM_Siege_Trebuchet_A` and `SM_Siege_BatteringRam_A`.

Use plausible late-medieval timber joinery, iron straps, rope, and wheel construction. Keep the
trebuchet arm, axle, counterweight, sling anchors, and wheels as separate named children. Keep the
ram log, suspension ropes, wheels, and roof frame as separate named children. Make both assets
silhouette-readable from the default game camera.

For each root:

- match manifest dimensions within 5%;
- apply rotation and scale on exportable mesh objects;
- ground-center pivot, facing +X;
- UV0 with no accidental overlaps outside intentionally mirrored/tiled pieces;
- simple UCX collision, not render-mesh collision;
- LOD children with suffixes `_LOD1` and `_LOD2`;
- save and inspect four views before continuing.

Do not export until the validation prompt is completed.
