# Paste-ready MCP prompt — validate and export

First save the current `.blend` file. Review the local script
`BlenderMCP/scripts/ue_asset_pipeline.py` before executing it. The script may inspect Blender scene
data and write FBX files only to the export directory I explicitly provide; do not permit any
other filesystem, network, shell, package-install, or subprocess action.

Run `validate_scene()` and return every error and warning grouped by asset. Do not export while
any error remains. Fix errors one asset at a time and rerun validation.

After validation is clean, ask me for an absolute export directory. Then call
`export_assets(<approved absolute directory>)`. Use FBX with selected objects, mesh types only,
applied unit scale, `-Y` forward, `Z` up, tangent space, and custom properties. Export each root
asset with its UCX collision objects to the exact filename in the manifest.

Finally return:

- exported filename and absolute path;
- root dimensions;
- LOD availability;
- material slots;
- collision object names;
- validation status.
