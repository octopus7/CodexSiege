"""
Create the complete WBP_SailFleetHUD widget tree.

Run after the AgeOfSailFleet C++ module has compiled:

    UnrealEditor-Cmd.exe AgeOfSailFleet.uproject ^
      -ExecutePythonScript=Content/Python/create_ui_assets.py

The native USailFleetHUDWidget class never creates widgets. This script authors and
saves all eight ship cards in the Widget Blueprint so opening WBP_SailFleetHUD in
Designer always shows the complete hierarchy.
"""

import unreal


ASSET_PATH = "/Game/UI"
ASSET_NAME = "WBP_SailFleetHUD"
PARENT_CLASS_PATH = "/Script/AgeOfSailFleet.SailFleetHUDWidget"
TITLE_ASSET_NAME = "WBP_TitleScreen"
TITLE_PARENT_CLASS_PATH = "/Script/AgeOfSailFleet.SailTitleScreenWidget"
MAX_CARDS = 8

INK = unreal.LinearColor(0.025, 0.020, 0.015, 1.0)
PARCHMENT = unreal.LinearColor(0.79, 0.69, 0.47, 1.0)
GOLD = unreal.LinearColor(0.76, 0.51, 0.16, 1.0)
DARK_GOLD = unreal.LinearColor(0.28, 0.16, 0.045, 1.0)
PANEL = unreal.LinearColor(0.035, 0.029, 0.022, 0.97)
BLUE = unreal.LinearColor(0.055, 0.20, 0.58, 1.0)
RED = unreal.LinearColor(0.60, 0.055, 0.045, 1.0)


def _set(obj, prop, value):
    """Set an editor property while tolerating minor UE Python API renames."""
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception:
        return False


def _call(obj, method_name, *args):
    method = getattr(obj, method_name, None)
    if method is None:
        return None
    try:
        return method(*args)
    except Exception as exc:
        unreal.log_warning("{} failed on {}: {}".format(method_name, obj.get_name(), exc))
        return None


def _margin(left=0.0, top=0.0, right=0.0, bottom=0.0):
    value = unreal.Margin()
    _set(value, "left", float(left))
    _set(value, "top", float(top))
    _set(value, "right", float(right))
    _set(value, "bottom", float(bottom))
    return value


def _anchors(min_x, min_y, max_x, max_y):
    value = unreal.Anchors()
    _set(value, "minimum", unreal.Vector2D(min_x, min_y))
    _set(value, "maximum", unreal.Vector2D(max_x, max_y))
    return value


def _configure_canvas_slot(
    slot,
    anchors,
    alignment,
    offsets,
    auto_size=False,
    z_order=None,
):
    """Author CanvasPanelSlot layout through its UE 5.7 setter API.

    CanvasPanelSlot does not expose anchors/alignment/offsets as independently
    writable editor properties in every engine build.  Silently writing those
    names left the generated WBP children at their 100x30 top-left defaults.
    """
    slot.set_anchors(anchors)
    slot.set_alignment(alignment)
    slot.set_offsets(offsets)
    slot.set_auto_size(bool(auto_size))
    if z_order is not None:
        slot.set_z_order(int(z_order))


def _slate_color(color):
    result = unreal.SlateColor()
    _set(result, "specified_color", color)
    return result


def _enum(enum_type, *names):
    for name in names:
        if hasattr(enum_type, name):
            return getattr(enum_type, name)
    return None


def _load_ui_font():
    # Drop any OFL/Apache licensed antique font at the first path to upgrade the
    # look without changing the WBP script. Engine Roboto is the offline fallback.
    candidates = (
        "/Game/UI/Fonts/FleetAntiqueRuntime",
        "/Game/UI/Fonts/FleetAntique",
        "/Engine/EngineFonts/RobotoDistanceField",
        "/Engine/EngineFonts/Roboto",
    )
    for path in candidates:
        asset = unreal.EditorAssetLibrary.load_asset(path)
        # A raw FontFace does not implement IFontProviderInterface. Assigning
        # one directly makes Slate display its LastResort "A" placeholder.
        if asset and isinstance(asset, unreal.Font):
            unreal.log("Fleet HUD font: {}".format(path))
            return asset
    return None


UI_FONT = _load_ui_font()


def _font(size, outline=1):
    info = unreal.SlateFontInfo()
    _set(info, "size", int(size))
    if UI_FONT:
        _set(info, "font_object", UI_FONT)
        _set(info, "typeface_font_name", "Regular")
    outline_settings = unreal.FontOutlineSettings()
    _set(outline_settings, "outline_size", int(outline))
    _set(outline_settings, "outline_color", INK)
    _set(outline_settings, "separate_fill_alpha", True)
    _set(info, "outline_settings", outline_settings)
    return info


def _construct(tree, widget_class, name):
    widget = unreal.SailFleetUIEditorLibrary.construct_widget(
        tree, widget_class, name
    )
    if not widget:
        raise RuntimeError("Could not construct {}".format(name))
    _set(widget, "is_variable", True)
    return widget


def _add(parent, child):
    result = parent.add_child(child)
    if result is None:
        # Content widgets expose set_content on some engine versions.
        _call(parent, "set_content", child)
        return child.slot
    return result


def _set_padding(slot_or_widget, margin):
    if not _set(slot_or_widget, "padding", margin):
        _call(slot_or_widget, "set_padding", margin)


def _text(tree, name, value, size, color=PARCHMENT, outline=1):
    widget = _construct(tree, unreal.TextBlock, name)
    _call(widget, "set_text", value)
    if not _set(widget, "font", _font(size, outline)):
        _call(widget, "set_font", _font(size, outline))
    if not _set(widget, "color_and_opacity", _slate_color(color)):
        _call(widget, "set_color_and_opacity", _slate_color(color))
    _set(widget, "shadow_offset", unreal.Vector2D(1.5, 1.5))
    _set(widget, "shadow_color_and_opacity", unreal.LinearColor(0.0, 0.0, 0.0, 0.9))
    justification = _enum(unreal.TextJustify, "CENTER")
    if justification is not None:
        _set(widget, "justification", justification)
    return widget


def _border(tree, name, color, padding=None):
    widget = _construct(tree, unreal.Border, name)
    _call(widget, "set_brush_color", color)
    if padding is not None:
        if not _set(widget, "padding", padding):
            _call(widget, "set_padding", padding)
    return widget


def _image(tree, name, texture_path=None, color=None):
    widget = _construct(tree, unreal.Image, name)
    if texture_path:
        texture = None
        candidates = (
            texture_path if isinstance(texture_path, (tuple, list)) else (texture_path,)
        )
        for candidate in candidates:
            texture = unreal.EditorAssetLibrary.load_asset(candidate)
            if texture:
                break
        if texture:
            _call(widget, "set_brush_from_texture", texture, True)
    _call(
        widget,
        "set_color_and_opacity",
        color or unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
    )
    return widget


def _glyph_slot(tree, base_name, texture_path, width, height):
    size = _size(tree, "{}Size".format(base_name), width, height)
    overlay = _construct(tree, unreal.Overlay, "{}Overlay".format(base_name))
    _add(size, overlay)
    glow = _image(
        tree,
        "{}Glow".format(base_name),
        texture_path,
        unreal.LinearColor(0.0, 0.0, 0.0, 0.88),
    )
    glow_transform = unreal.WidgetTransform()
    _set(glow_transform, "scale", unreal.Vector2D(1.08, 1.08))
    _set(glow, "render_transform", glow_transform)
    _call(glow, "set_desired_size_override", unreal.Vector2D(width, height))
    glow_slot = overlay.add_child_to_overlay(glow)
    horizontal_fill = _enum(
        unreal.HorizontalAlignment,
        "H_ALIGN_FILL",
        "FILL",
    )
    vertical_fill = _enum(
        unreal.VerticalAlignment,
        "V_ALIGN_FILL",
        "FILL",
    )
    if horizontal_fill is not None:
        _set(glow_slot, "horizontal_alignment", horizontal_fill)
    if vertical_fill is not None:
        _set(glow_slot, "vertical_alignment", vertical_fill)
    main = _image(
        tree,
        "{}Image".format(base_name),
        texture_path,
        unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
    )
    _call(main, "set_desired_size_override", unreal.Vector2D(width, height))
    main_slot = overlay.add_child_to_overlay(main)
    if horizontal_fill is not None:
        _set(main_slot, "horizontal_alignment", horizontal_fill)
    if vertical_fill is not None:
        _set(main_slot, "vertical_alignment", vertical_fill)
    return size


def _size(tree, name, width=None, height=None):
    widget = _construct(tree, unreal.SizeBox, name)
    if width is not None:
        _call(widget, "set_width_override", float(width))
    if height is not None:
        _call(widget, "set_height_override", float(height))
    return widget


def _fill_vertical_slot(slot, padding=None):
    if padding:
        _set_padding(slot, padding)
    fill = _enum(unreal.SlateSizeRule, "FILL")
    if fill is not None:
        _set(slot, "size", unreal.SlateChildSize(value=1.0, size_rule=fill))


def _build_card(tree, card_number):
    suffix = "{:02d}".format(card_number)
    size_box = _size(tree, "CardSize{}".format(suffix), 178.0, 226.0)

    # CardContainer is only the native visibility root. Its brush is fully
    # transparent: there is deliberately no rectangular card or panel.
    container = _border(
        tree,
        "CardContainer{}".format(suffix),
        unreal.LinearColor(0.0, 0.0, 0.0, 0.0),
    )
    _add(size_box, container)

    content = _construct(tree, unreal.VerticalBox, "CardContent{}".format(suffix))
    _add(container, content)

    portrait_size = _size(tree, "PortraitSize{}".format(suffix), 166.0, 170.0)
    portrait_slot = content.add_child_to_vertical_box(portrait_size)
    _set_padding(portrait_slot, _margin(6, 0, 6, 0))
    center = _enum(unreal.HorizontalAlignment, "H_ALIGN_CENTER", "CENTER")
    if center is not None:
        _set(portrait_slot, "horizontal_alignment", center)

    portrait_overlay = _construct(tree, unreal.Overlay, "PortraitOverlay{}".format(suffix))
    _add(portrait_size, portrait_overlay)

    # The glow uses the same alpha silhouette as the chosen rank locket, so
    # faction color never creates a rectangular backing plate.
    faction_glow = _image(
        tree,
        "FactionGlow{}".format(suffix),
        "/Game/UI/Lockets/T_Locket_Gold",
        BLUE if card_number % 2 else RED,
    )
    glow_transform = unreal.WidgetTransform()
    _set(glow_transform, "scale", unreal.Vector2D(1.045, 1.045))
    _set(glow_transform, "translation", unreal.Vector2D(-3.7, -3.7))
    _set(faction_glow, "render_transform", glow_transform)
    _call(faction_glow, "set_render_opacity", 0.58)
    portrait_overlay.add_child_to_overlay(faction_glow)

    portrait = _image(
        tree,
        "Portrait{}".format(suffix),
        "/Game/UI/Captains/T_Captain_Blue_Admiral_Ward_Oval",
        unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
    )
    # The authored oval mask is slightly smaller than the locket's inner rim.
    # Overscan it beneath the frame so no transparent seam can show around the
    # portrait; the frame is added afterward and therefore remains on top.
    portrait_transform = unreal.WidgetTransform()
    _set(portrait_transform, "scale", unreal.Vector2D(1.10, 1.10))
    _set(portrait, "render_transform", portrait_transform)
    _set(portrait, "render_transform_pivot", unreal.Vector2D(0.5, 0.5))
    portrait_overlay.add_child_to_overlay(portrait)

    locket_frame = _image(
        tree,
        "LocketFrame{}".format(suffix),
        "/Game/UI/Lockets/T_Locket_Gold",
        unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
    )
    portrait_overlay.add_child_to_overlay(locket_frame)

    captain_name = _text(
        tree,
        "CaptainName{}".format(suffix),
        "CAPT. E. HARCOURT",
        12,
        unreal.LinearColor(0.95, 0.83, 0.55, 1.0),
        1,
    )
    captain_slot = content.add_child_to_vertical_box(captain_name)
    _set_padding(captain_slot, _margin(1, -3, 1, 0))

    ship_name = _text(
        tree,
        "ShipName{}".format(suffix),
        "HMS RESOLUTE",
        10,
        unreal.LinearColor(0.76, 0.71, 0.60, 1.0),
        1,
    )
    ship_slot = content.add_child_to_vertical_box(ship_name)
    _set_padding(ship_slot, _margin(2, 0, 2, 0))

    ship_class = _text(
        tree,
        "ShipClass{}".format(suffix),
        "FIRST-RATE | 100 GUNS",
        8,
        unreal.LinearColor(0.63, 0.56, 0.42, 1.0),
        1,
    )
    ship_class_slot = content.add_child_to_vertical_box(ship_class)
    _set_padding(ship_class_slot, _margin(2, 0, 2, 0))
    # Hidden (rather than Collapsed) deliberately keeps the original vertical
    # allocation below the ship name while omitting this detail line.
    hidden = _enum(unreal.SlateVisibility, "HIDDEN")
    if hidden is not None:
        _call(ship_class, "set_visibility", hidden)

    rank_text = _text(
        tree,
        "RankText{}".format(suffix),
        "III  FLAGSHIP",
        8,
        GOLD,
        1,
    )
    rank_slot = content.add_child_to_vertical_box(rank_text)
    _set_padding(rank_slot, _margin(2, 0, 2, 0))
    if hidden is not None:
        _call(rank_text, "set_visibility", hidden)

    health = _construct(tree, unreal.ProgressBar, "HealthBar{}".format(suffix))
    _call(health, "set_percent", 0.78)
    _call(health, "set_fill_color_and_opacity", unreal.LinearColor(0.12, 0.46, 0.23, 1.0))
    health_slot = content.add_child_to_vertical_box(health)
    _set_padding(health_slot, _margin(24, 1, 24, 0))

    return size_box


def _clear_tree(tree):
    unreal.SailFleetUIEditorLibrary.clear_widget_tree(tree)


def _set_root(tree, root):
    unreal.SailFleetUIEditorLibrary.set_root_widget(tree, root)


def _find_widget(tree, name):
    return unreal.SailFleetUIEditorLibrary.find_widget(tree, name)


def _get_or_create_blueprint(asset_name=ASSET_NAME, parent_class_path=PARENT_CLASS_PATH):
    full_path = "{}/{}".format(ASSET_PATH, asset_name)
    existing = unreal.EditorAssetLibrary.load_asset(full_path)
    if existing:
        unreal.log("Rebuilding existing {}".format(full_path))
        return existing

    parent_class = unreal.load_class(None, parent_class_path)
    if not parent_class:
        raise RuntimeError(
            "Native class was not found. Compile AgeOfSailFleet, restart the editor, "
            "then run this script again: {}".format(parent_class_path)
        )

    unreal.EditorAssetLibrary.make_directory(ASSET_PATH)
    factory = unreal.WidgetBlueprintFactory()
    _set(factory, "parent_class", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, ASSET_PATH, unreal.WidgetBlueprint, factory
    )
    if not blueprint:
        raise RuntimeError("Unable to create {}/{}".format(ASSET_PATH, asset_name))
    return blueprint


def _build_blueprint():
    blueprint = _get_or_create_blueprint()
    tree = unreal.SailFleetUIEditorLibrary.get_widget_tree(blueprint)
    if not tree:
        raise RuntimeError("Editor bridge could not access WBP_SailFleetHUD WidgetTree")
    _clear_tree(tree)

    root = _construct(tree, unreal.CanvasPanel, "FleetHUDRoot")
    _set_root(tree, root)

    # The selected fleet sits directly over the world. There is intentionally no
    # bottom panel, shadow rectangle, header, or selection-count text.
    row_scale = _construct(tree, unreal.ScaleBox, "ShipCardScale")
    scale_slot = root.add_child_to_canvas(row_scale)
    _configure_canvas_slot(
        scale_slot,
        _anchors(0.5, 1.0, 0.5, 1.0),
        unreal.Vector2D(0.5, 1.0),
        _margin(0, -10, 1495, 232),
        z_order=1,
    )
    stretch = _enum(unreal.Stretch, "SCALE_TO_FIT", "SCALE_TO_FIT_X")
    if stretch is not None:
        _set(row_scale, "stretch", stretch)
    direction = _enum(unreal.StretchDirection, "DOWN_ONLY", "BOTH")
    if direction is not None:
        _set(row_scale, "stretch_direction", direction)

    ship_row = _construct(tree, unreal.HorizontalBox, "ShipCardRow")
    _add(row_scale, ship_row)
    for card_number in range(1, MAX_CARDS + 1):
        card = _build_card(tree, card_number)
        card_slot = ship_row.add_child_to_horizontal_box(card)
        _set_padding(card_slot, _margin(4, 0, 4, 0))

    # Date and wind are image-only displays. The transparent binding container
    # keeps the glyphs directly over the sea with no rectangular panel behind them.
    status_panel = _border(
        tree,
        "DateWindPanel",
        unreal.LinearColor(0.0, 0.0, 0.0, 0.0),
        _margin(0, 0, 0, 0),
    )
    status_slot = root.add_child_to_canvas(status_panel)
    _configure_canvas_slot(
        status_slot,
        _anchors(1.0, 0.0, 1.0, 0.0),
        unreal.Vector2D(1.0, 0.0),
        _margin(-28, 28, 610, 120),
        z_order=20,
    )

    # Use an authored canvas for deterministic glyph geometry. Horizontal/vertical
    # box desired-size negotiation can collapse texture-only children to zero in a
    # cooked viewport even though they remain visible in the Widget Designer.
    status_content = _construct(tree, unreal.CanvasPanel, "DateWindCanvas")
    _add(status_panel, status_content)
    # Preserve each cropped source texture's natural aspect ratio at 34 px high.
    # The former oversized widths stretched glyph faces horizontally and read as
    # excessive tracking; the small inter-slot gap below supplies the separation.
    date_specs = (
        (
            "DateWeekday",
            (
                "/Game/UI/DateGlyphs/T_Date_Weekday_FRI",
                "/Game/UI/DateGlyphs/T_Date_Weekday_Friday",
            ),
            48,
            34,
        ),
        ("DatePunctuation", "/Game/UI/DateGlyphs/T_Date_Punctuation_Dot", 34, 34),
        ("DateDayTens", "/Game/UI/DateGlyphs/T_Date_Digit_2", 24, 34),
        ("DateDayOnes", "/Game/UI/DateGlyphs/T_Date_Digit_4", 28, 34),
        (
            "DateMonth",
            (
                "/Game/UI/DateGlyphs/T_Date_Month_JULY",
                "/Game/UI/DateGlyphs/T_Date_Month_July",
            ),
            74,
            34,
        ),
        ("DateYearThousands", "/Game/UI/DateGlyphs/T_Date_Digit_1", 18, 34),
        ("DateYearHundreds", "/Game/UI/DateGlyphs/T_Date_Digit_7", 25, 34),
        ("DateYearTens", "/Game/UI/DateGlyphs/T_Date_Digit_1", 18, 34),
        ("DateYearOnes", "/Game/UI/DateGlyphs/T_Date_Digit_5", 25, 34),
    )
    date_x = 0.0
    for base_name, texture_path, width, height in date_specs:
        glyph = _glyph_slot(tree, base_name, texture_path, width, height)
        glyph_slot = status_content.add_child_to_canvas(glyph)
        _configure_canvas_slot(
            glyph_slot,
            _anchors(0.0, 0.0, 0.0, 0.0),
            unreal.Vector2D(0.0, 0.0),
            _margin(date_x, 0, width, height),
            z_order=2,
        )
        date_x += width + 2.0

    wind_specs = (
        ("WindArrow", "/Game/UI/DateGlyphs/T_Wind_Arrow", 42, 42),
    )
    wind_x = (date_x - 2.0 - 42.0) * 0.5
    for base_name, texture_path, width, height in wind_specs:
        glyph = _glyph_slot(tree, base_name, texture_path, width, height)
        glyph_slot = status_content.add_child_to_canvas(glyph)
        _configure_canvas_slot(
            glyph_slot,
            _anchors(0.0, 0.0, 0.0, 0.0),
            unreal.Vector2D(0.0, 0.0),
            _margin(wind_x, 48, width, height),
            z_order=2,
        )
        wind_x += width + 8.0

    required = ["ShipCardRow"]
    for card_number in range(1, MAX_CARDS + 1):
        suffix = "{:02d}".format(card_number)
        required.extend(
            "{}{}".format(prefix, suffix)
            for prefix in (
                "CardContainer",
                "FactionGlow",
                "LocketFrame",
                "Portrait",
                "CaptainName",
                "ShipName",
                "ShipClass",
                "RankText",
                "HealthBar",
            )
        )
    required.extend(
        "{}{}".format(base_name, suffix)
        for base_name in (
            "DateWeekday",
            "DatePunctuation",
            "DateDayTens",
            "DateDayOnes",
            "DateMonth",
            "DateYearThousands",
            "DateYearHundreds",
            "DateYearTens",
            "DateYearOnes",
            "WindArrow",
        )
        for suffix in ("Glow", "Image")
    )
    missing = [name for name in required if not _find_widget(tree, name)]
    if missing:
        raise RuntimeError("WBP tree validation failed: {}".format(", ".join(missing)))

    unreal.SailFleetUIEditorLibrary.compile_widget_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    unreal.log(
        "Created /Game/UI/WBP_SailFleetHUD with {} complete authored ship cards.".format(
            MAX_CARDS
        )
    )
    return blueprint


def _build_title_blueprint():
    blueprint = _get_or_create_blueprint(TITLE_ASSET_NAME, TITLE_PARENT_CLASS_PATH)
    tree = unreal.SailFleetUIEditorLibrary.get_widget_tree(blueprint)
    if not tree:
        raise RuntimeError("Editor bridge could not access WBP_TitleScreen WidgetTree")
    _clear_tree(tree)

    root = _construct(tree, unreal.CanvasPanel, "TitleScreenRoot")
    _set_root(tree, root)

    # The title composition floats directly over the live ocean. TitlePanel is
    # transparent and exists only as the native fade-animation binding root.
    panel_size = _size(tree, "TitlePanelSize", 820.0, 690.0)
    panel_slot = root.add_child_to_canvas(panel_size)
    _configure_canvas_slot(
        panel_slot,
        _anchors(0.5, 0.5, 0.5, 0.5),
        unreal.Vector2D(0.5, 0.5),
        _margin(0, 0, 820, 690),
        z_order=1,
    )

    title_panel = _border(
        tree,
        "TitlePanel",
        unreal.LinearColor(0.0, 0.0, 0.0, 0.0),
        _margin(18, 8, 18, 8),
    )
    _add(panel_size, title_panel)

    content = _construct(tree, unreal.VerticalBox, "TitleContent")
    _add(title_panel, content)

    crest = _text(
        tree,
        "TitleCrest",
        "-=-  ADMIRALTY OF THE HIGH SEAS  -=-",
        24,
        unreal.LinearColor(0.72, 0.50, 0.18, 0.95),
        1,
    )
    crest_slot = content.add_child_to_vertical_box(crest)
    _set_padding(crest_slot, _margin(0, 0, 0, 6))

    logo = _image(
        tree,
        "LogoImage",
        "/Game/UI/Title/T_AgeOfSail_Logo",
        unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
    )
    logo_brush = logo.get_editor_property("brush")
    _set(logo_brush, "draw_as", _enum(unreal.SlateBrushDrawType, "IMAGE"))
    _set(logo_brush, "image_size", unreal.Vector2D(2172, 724))
    _set(logo, "brush", logo_brush)
    logo_size = _size(tree, "LogoImageSize", 700, 234)
    # The source and authored box are both exactly 3:1, so a direct fill keeps
    # the logo aspect while avoiding ScaleBox's zero desired-size edge case.
    _add(logo_size, logo)
    logo_slot = content.add_child_to_vertical_box(logo_size)
    _set_padding(logo_slot, _margin(42, 0, 42, 10))

    # Retain the former ScaleBox node as a collapsed authored compatibility node.
    # UE keeps removed widget objects alive until GC; preserving the named node
    # avoids stale variable diagnostics while the visible logo remains direct.
    legacy_logo_scale = _construct(tree, unreal.ScaleBox, "LogoScaleBox")
    legacy_logo_spacer = _construct(tree, unreal.Spacer, "LogoScaleCompatibilitySpacer")
    _add(legacy_logo_scale, legacy_logo_spacer)
    legacy_logo_slot = content.add_child_to_vertical_box(legacy_logo_scale)
    _set(
        legacy_logo_scale,
        "visibility",
        _enum(unreal.SlateVisibility, "COLLAPSED"),
    )

    rule = _border(tree, "TitleGildedRule", unreal.LinearColor(0.70, 0.46, 0.13, 0.82))
    rule_size = _size(tree, "TitleGildedRuleSize", None, 2)
    _add(rule_size, rule)
    rule_slot = content.add_child_to_vertical_box(rule_size)
    _set_padding(rule_slot, _margin(105, 0, 105, 16))

    description = _text(
        tree,
        "TitleDescriptionText",
        (
            "Command a fleet of wooden ships of the line.\n"
            "Read the wind, hold formation, and open the gunports."
        ),
        17,
        unreal.LinearColor(0.78, 0.72, 0.60, 1.0),
        1,
    )
    _set(description, "auto_wrap_text", True)
    _set(description, "wrap_text_at", 710.0)
    description_slot = content.add_child_to_vertical_box(description)
    _set_padding(description_slot, _margin(50, 4, 50, 15))

    mode_row = _construct(tree, unreal.HorizontalBox, "GraphicModeRow")
    mode_row_slot = content.add_child_to_vertical_box(mode_row)
    _set_padding(mode_row_slot, _margin(170, 0, 170, 14))

    mode_label = _text(
        tree,
        "GraphicModeLabel",
        "GRAPHIC MODE",
        16,
        unreal.LinearColor(0.78, 0.55, 0.22, 1.0),
        1,
    )
    label_slot = mode_row.add_child_to_horizontal_box(mode_label)
    _set_padding(label_slot, _margin(0, 7, 30, 0))

    mode_options = _construct(tree, unreal.HorizontalBox, "GraphicModeOptions")
    options_slot = mode_row.add_child_to_horizontal_box(mode_options)
    _set_padding(options_slot, _margin(0, 0, 0, 0))

    def radio_brush(fill_color, outline_color, outline_width):
        brush = unreal.SlateBrush()
        _set(
            brush,
            "draw_as",
            _enum(unreal.SlateBrushDrawType, "ROUNDED_BOX"),
        )
        _set(brush, "image_size", unreal.Vector2D(24, 24))
        _set(brush, "tint_color", _slate_color(fill_color))
        outline = unreal.SlateBrushOutlineSettings()
        _set(
            outline,
            "rounding_type",
            _enum(unreal.SlateBrushRoundingType, "HALF_HEIGHT_RADIUS"),
        )
        _set(outline, "color", _slate_color(outline_color))
        _set(outline, "width", float(outline_width))
        _set(brush, "outline_settings", outline)
        return brush

    radio_style = unreal.CheckBoxStyle()
    for brush_name, brush in (
        (
            "unchecked_image",
            radio_brush(
                unreal.LinearColor(0.02, 0.015, 0.01, 0.48),
                unreal.LinearColor(0.76, 0.51, 0.16, 1.0),
                2.0,
            ),
        ),
        (
            "unchecked_hovered_image",
            radio_brush(
                unreal.LinearColor(0.10, 0.065, 0.025, 0.72),
                unreal.LinearColor(0.95, 0.75, 0.32, 1.0),
                2.5,
            ),
        ),
        (
            "unchecked_pressed_image",
            radio_brush(
                unreal.LinearColor(0.18, 0.10, 0.025, 0.88),
                unreal.LinearColor(0.98, 0.79, 0.36, 1.0),
                2.5,
            ),
        ),
        (
            "checked_image",
            radio_brush(
                unreal.LinearColor(0.88, 0.61, 0.18, 1.0),
                unreal.LinearColor(0.18, 0.09, 0.02, 1.0),
                3.5,
            ),
        ),
        (
            "checked_hovered_image",
            radio_brush(
                unreal.LinearColor(1.0, 0.78, 0.31, 1.0),
                unreal.LinearColor(0.23, 0.12, 0.025, 1.0),
                3.5,
            ),
        ),
        (
            "checked_pressed_image",
            radio_brush(
                unreal.LinearColor(0.72, 0.43, 0.10, 1.0),
                unreal.LinearColor(0.13, 0.065, 0.015, 1.0),
                3.5,
            ),
        ),
    ):
        _set(radio_style, brush_name, brush)
    _set(radio_style, "padding", _margin(0, 0, 7, 0))
    _set(radio_style, "foreground_color", _slate_color(PARCHMENT))
    _set(
        radio_style,
        "hovered_foreground",
        _slate_color(unreal.LinearColor(0.98, 0.82, 0.45, 1.0)),
    )
    _set(
        radio_style,
        "checked_foreground",
        _slate_color(unreal.LinearColor(0.98, 0.82, 0.45, 1.0)),
    )

    mode_3d = _construct(tree, unreal.CheckBox, "GraphicMode3DCheckBox")
    _set(mode_3d, "widget_style", radio_style)
    _call(mode_3d, "set_is_checked", True)
    mode_3d_label = _text(
        tree,
        "GraphicMode3DLabel",
        "3D",
        18,
        PARCHMENT,
        1,
    )
    _add(mode_3d, mode_3d_label)
    mode_3d_slot = mode_options.add_child_to_horizontal_box(mode_3d)
    _set_padding(mode_3d_slot, _margin(0, 3, 30, 0))

    mode_2d = _construct(tree, unreal.CheckBox, "GraphicMode2DCheckBox")
    _set(mode_2d, "widget_style", radio_style)
    _call(mode_2d, "set_is_checked", False)
    mode_2d_label = _text(
        tree,
        "GraphicMode2DLabel",
        "2D",
        18,
        PARCHMENT,
        1,
    )
    _add(mode_2d, mode_2d_label)
    mode_2d_slot = mode_options.add_child_to_horizontal_box(mode_2d)
    _set_padding(mode_2d_slot, _margin(0, 3, 0, 0))

    spacer = _construct(tree, unreal.Spacer, "TitleFlexibleSpacer")
    spacer_slot = content.add_child_to_vertical_box(spacer)
    _fill_vertical_slot(spacer_slot)

    departure_size = _size(tree, "DepartureButtonSize", 470, 157)
    departure_size_slot = content.add_child_to_vertical_box(departure_size)
    _set_padding(departure_size_slot, _margin(157, 6, 157, 5))
    departure_button = _construct(tree, unreal.Button, "DepartureButton")
    _call(departure_button, "set_background_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    _call(departure_button, "set_color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    departure_texture = unreal.EditorAssetLibrary.load_asset(
        "/Game/UI/Title/T_DepartureButton"
    )
    if departure_texture:
        button_style = departure_button.get_editor_property("widget_style")
        for brush_name, tint in (
            ("normal", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)),
            ("hovered", unreal.LinearColor(1.12, 1.08, 0.92, 1.0)),
            ("pressed", unreal.LinearColor(0.78, 0.75, 0.68, 1.0)),
            ("disabled", unreal.LinearColor(0.46, 0.46, 0.46, 0.72)),
        ):
            brush = button_style.get_editor_property(brush_name)
            _set(brush, "resource_object", departure_texture)
            _set(brush, "draw_as", _enum(unreal.SlateBrushDrawType, "IMAGE"))
            _set(brush, "image_size", unreal.Vector2D(470, 157))
            _set(brush, "tint_color", _slate_color(tint))
            _set(button_style, brush_name, brush)
        _set(button_style, "normal_padding", _margin(0, 0, 0, 0))
        _set(button_style, "pressed_padding", _margin(0, 2, 0, 0))
        _set(departure_button, "widget_style", button_style)
    _add(departure_size, departure_button)

    departure_label = _text(
        tree,
        "DepartureButtonText",
        "SET  SAIL",
        25,
        unreal.LinearColor(0.96, 0.81, 0.43, 1.0),
        2,
    )
    _set_padding(departure_label, _margin(18, 45, 18, 42))
    _add(departure_button, departure_label)

    footer = _text(
        tree,
        "TitleFooterText",
        "THE WIND FAVOURS THE BOLD",
        11,
        unreal.LinearColor(0.48, 0.40, 0.28, 0.88),
        1,
    )
    footer_slot = content.add_child_to_vertical_box(footer)
    _set_padding(footer_slot, _margin(0, 2, 0, 0))

    black_overlay = _border(
        tree, "FullscreenBlackOverlay", unreal.LinearColor(0.0, 0.0, 0.0, 1.0)
    )
    _call(black_overlay, "set_render_opacity", 0.0)
    black_slot = root.add_child_to_canvas(black_overlay)
    _configure_canvas_slot(
        black_slot,
        _anchors(0.0, 0.0, 1.0, 1.0),
        unreal.Vector2D(0.0, 0.0),
        _margin(0, 0, 0, 0),
        z_order=2,
    )

    missing_title = [
        name
        for name in (
            "DepartureButton",
            "GraphicMode3DCheckBox",
            "GraphicMode2DCheckBox",
            "TitlePanel",
            "LogoImage",
            "LogoScaleBox",
            "FullscreenBlackOverlay",
        )
        if not _find_widget(tree, name)
    ]
    if missing_title:
        raise RuntimeError(
            "WBP_TitleScreen validation failed: {}".format(", ".join(missing_title))
        )

    unreal.SailFleetUIEditorLibrary.compile_widget_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    unreal.log("Created /Game/UI/WBP_TitleScreen with a complete authored widget tree.")
    return blueprint


if __name__ == "__main__":
    _build_blueprint()
    _build_title_blueprint()
