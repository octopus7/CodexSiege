# Paste-ready MCP prompt — fortress kit

Read `BlenderMCP/manifests/siege_asset_manifest.json` and model only the three fortress assets:
`SM_Fortress_Wall_A`, `SM_Fortress_Gatehouse_A`, and `SM_Fortress_Tower_A`.

Work one asset at a time. Before each asset, report the target dimensions. After each asset:

1. Apply rotation and scale.
2. Put the origin at ground center.
3. Confirm normals face outward, there are no non-manifold accidental edges, and UV0 exists.
4. Add simple custom collision objects named `UCX_<asset>_01` in the `COLLISION` collection.
5. Keep logical moving pieces separate but parented.
6. Save the `.blend` file.
7. Use viewport screenshots to visually inspect front, rear, side, and three-quarter views.

The wall must tile exactly along local X. The gatehouse and tower connection edges must align to
the wall depth. Preserve all exact root object names from the manifest.
