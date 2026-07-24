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
    overlay.add_child_to_overlay(glow)
    main = _image(
        tree,
        "{}Image".format(base_name),
        texture_path,
        unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
    )
    _call(main, "set_desired_size_override", unreal.Vector2D(width, height))
    overlay.add_child_to_overlay(main)
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

    # Date and wind are image-only displays. Every glyph has a dark enlarged copy
    # under the white face image, producing an outer-glow treatment without text.
    status_panel = _border(
        tree,
        "DateWindPanel",
        unreal.LinearColor(0.0, 0.0, 0.0, 0.0),
        _margin(12, 8, 12, 8),
    )
    status_slot = root.add_child_to_canvas(status_panel)
    _configure_canvas_slot(
        status_slot,
        _anchors(1.0, 0.0, 1.0, 0.0),
        unreal.Vector2D(1.0, 0.0),
        _margin(-24, 24, 590, 112),
        z_order=2,
    )

    status_content = _construct(tree, unreal.VerticalBox, "DateWindContent")
    _add(status_panel, status_content)
    date_row = _construct(tree, unreal.HorizontalBox, "DateGlyphRow")
    status_content.add_child_to_vertical_box(date_row)
    date_specs = (
        (
            "DateWeekday",
            (
                "/Game/UI/DateGlyphs/T_Date_Weekday_FRI",
                "/Game/UI/DateGlyphs/T_Date_Weekday_Friday",
            ),
            146,
            34,
        ),
        ("DatePunctuation", "/Game/UI/DateGlyphs/T_Date_Punctuation_Dot", 28, 34),
        ("DateDayTens", "/Game/UI/DateGlyphs/T_Date_Digit_2", 30, 34),
        ("DateDayOnes", "/Game/UI/DateGlyphs/T_Date_Digit_4", 30, 34),
        (
            "DateMonth",
            (
                "/Game/UI/DateGlyphs/T_Date_Month_JULY",
                "/Game/UI/DateGlyphs/T_Date_Month_July",
            ),
            108,
            34,
        ),
        ("DateYearThousands", "/Game/UI/DateGlyphs/T_Date_Digit_1", 28, 34),
        ("DateYearHundreds", "/Game/UI/DateGlyphs/T_Date_Digit_7", 28, 34),
        ("DateYearTens", "/Game/UI/DateGlyphs/T_Date_Digit_1", 28, 34),
        ("DateYearOnes", "/Game/UI/DateGlyphs/T_Date_Digit_5", 28, 34),
    )
    for base_name, texture_path, width, height in date_specs:
        glyph = _glyph_slot(tree, base_name, texture_path, width, height)
        glyph_slot = date_row.add_child_to_horizontal_box(glyph)
        _set_padding(glyph_slot, _margin(3, 0, 3, 0))

    wind_row = _construct(tree, unreal.HorizontalBox, "WindGlyphRow")
    wind_row_slot = status_content.add_child_to_vertical_box(wind_row)
    _set_padding(wind_row_slot, _margin(90, 9, 0, 0))
    wind_specs = (
        ("WindCompass", "/Game/UI/DateGlyphs/T_Wind_Compass", 42, 42),
        ("WindArrow", "/Game/UI/DateGlyphs/T_Wind_Arrow", 42, 42),
    )
    for base_name, texture_path, width, height in wind_specs:
        glyph = _glyph_slot(tree, base_name, texture_path, width, height)
        glyph_slot = wind_row.add_child_to_horizontal_box(glyph)
        _set_padding(glyph_slot, _margin(4, 0, 4, 0))

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
            "WindCompass",
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

    veil = _border(tree, "OceanVeil", unreal.LinearColor(0.012, 0.025, 0.035, 0.92))
    veil_slot = root.add_child_to_canvas(veil)
    _configure_canvas_slot(
        veil_slot,
        _anchors(0.0, 0.0, 1.0, 1.0),
        unreal.Vector2D(0.0, 0.0),
        _margin(0, 0, 0, 0),
        z_order=0,
    )

    # Multiple translucent brush layers make a soft, alpha-edged ornamental frame
    # while remaining entirely editable in the WBP Designer.
    panel_size = _size(tree, "TitlePanelSize", 900.0, 730.0)
    panel_slot = root.add_child_to_canvas(panel_size)
    _configure_canvas_slot(
        panel_slot,
        _anchors(0.5, 0.5, 0.5, 0.5),
        unreal.Vector2D(0.5, 0.5),
        _margin(0, 0, 900, 730),
        z_order=1,
    )

    outer_glow = _border(
        tree,
        "TitlePanel",
        unreal.LinearColor(0.64, 0.42, 0.12, 0.30),
        _margin(7, 7, 7, 7),
    )
    _add(panel_size, outer_glow)
    outer_line = _border(
        tree,
        "TitleOuterLine",
        unreal.LinearColor(0.75, 0.52, 0.18, 0.78),
        _margin(3, 3, 3, 3),
    )
    _add(outer_glow, outer_line)
    inner_alpha = _border(
        tree,
        "TitleInnerAlphaFrame",
        unreal.LinearColor(0.15, 0.09, 0.035, 0.72),
        _margin(9, 9, 9, 9),
    )
    _add(outer_line, inner_alpha)
    title_panel = _border(
        tree,
        "TitleContentPanel",
        unreal.LinearColor(0.025, 0.027, 0.025, 0.94),
        _margin(30, 24, 30, 24),
    )
    _add(inner_alpha, title_panel)

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

    title = _text(
        tree,
        "GameTitleText",
        "AGE OF SAIL",
        56,
        unreal.LinearColor(0.94, 0.76, 0.35, 1.0),
        3,
    )
    title_slot = content.add_child_to_vertical_box(title)
    _set_padding(title_slot, _margin(0, 0, 0, 0))

    subtitle = _text(
        tree,
        "GameSubtitleText",
        "F L E E T   C O M M A N D",
        19,
        unreal.LinearColor(0.68, 0.56, 0.34, 1.0),
        1,
    )
    subtitle_slot = content.add_child_to_vertical_box(subtitle)
    _set_padding(subtitle_slot, _margin(0, 0, 0, 14))

    logo = _image(
        tree,
        "LogoImage",
        "/Game/UI/Title/T_AgeOfSail_Logo",
        unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
    )
    logo_size = _size(tree, "LogoImageSize", 650, 155)
    _add(logo_size, logo)
    logo_slot = content.add_child_to_vertical_box(logo_size)
    _set_padding(logo_slot, _margin(70, 0, 70, 12))

    rule = _border(tree, "TitleGildedRule", unreal.LinearColor(0.70, 0.46, 0.13, 0.82))
    rule_size = _size(tree, "TitleGildedRuleSize", None, 2)
    _add(rule_size, rule)
    rule_slot = content.add_child_to_vertical_box(rule_size)
    _set_padding(rule_slot, _margin(95, 0, 95, 18))

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
    _set_padding(description_slot, _margin(50, 6, 50, 16))

    mode_row = _construct(tree, unreal.HorizontalBox, "GraphicModeRow")
    mode_row_slot = content.add_child_to_vertical_box(mode_row)
    _set_padding(mode_row_slot, _margin(190, 0, 190, 18))

    mode_label = _text(
        tree,
        "GraphicModeLabel",
        "GRAPHIC MODE",
        16,
        unreal.LinearColor(0.78, 0.55, 0.22, 1.0),
        1,
    )
    label_slot = mode_row.add_child_to_horizontal_box(mode_label)
    _set_padding(label_slot, _margin(0, 9, 24, 0))

    combo_frame = _border(
        tree,
        "GraphicModeBronzeFrame",
        unreal.LinearColor(0.56, 0.31, 0.09, 1.0),
        _margin(2, 2, 2, 2),
    )
    combo_frame_size = _size(tree, "GraphicModeFrameSize", 230, 44)
    _add(combo_frame_size, combo_frame)
    combo_slot = mode_row.add_child_to_horizontal_box(combo_frame_size)
    _set_padding(combo_slot, _margin(0, 0, 0, 0))

    mode_combo = _construct(tree, unreal.ComboBoxString, "GraphicModeComboBox")
    _call(mode_combo, "add_option", "3D")
    _call(mode_combo, "add_option", "2D")
    _call(mode_combo, "set_selected_option", "3D")
    _set(mode_combo, "font", _font(17, 0))
    _set(mode_combo, "foreground_color", _slate_color(INK))
    _set(mode_combo, "content_padding", _margin(18, 8, 12, 8))

    combo_style = mode_combo.get_editor_property("widget_style")
    button_style = combo_style.get_editor_property("combo_button_style")
    inner_button = button_style.get_editor_property("button_style")
    for brush_name, color in (
        ("normal", unreal.LinearColor(0.76, 0.66, 0.45, 1.0)),
        ("hovered", unreal.LinearColor(0.87, 0.76, 0.52, 1.0)),
        ("pressed", unreal.LinearColor(0.66, 0.54, 0.34, 1.0)),
    ):
        brush = inner_button.get_editor_property(brush_name)
        _set(brush, "draw_as", _enum(unreal.SlateBrushDrawType, "BOX"))
        _set(brush, "tint_color", _slate_color(color))
        _set(inner_button, brush_name, brush)
    _set(button_style, "button_style", inner_button)
    _set(combo_style, "combo_button_style", button_style)
    _set(mode_combo, "widget_style", combo_style)

    item_style = mode_combo.get_editor_property("item_style")
    for brush_name, color in (
        ("even_row_background_brush", unreal.LinearColor(0.76, 0.66, 0.45, 1.0)),
        ("odd_row_background_brush", unreal.LinearColor(0.76, 0.66, 0.45, 1.0)),
        ("even_row_background_hovered_brush", unreal.LinearColor(0.87, 0.76, 0.52, 1.0)),
        ("odd_row_background_hovered_brush", unreal.LinearColor(0.87, 0.76, 0.52, 1.0)),
        ("active_brush", unreal.LinearColor(0.66, 0.54, 0.34, 1.0)),
        ("active_hovered_brush", unreal.LinearColor(0.87, 0.76, 0.52, 1.0)),
        ("inactive_brush", unreal.LinearColor(0.76, 0.66, 0.45, 1.0)),
        ("inactive_hovered_brush", unreal.LinearColor(0.87, 0.76, 0.52, 1.0)),
    ):
        brush = item_style.get_editor_property(brush_name)
        _set(brush, "draw_as", _enum(unreal.SlateBrushDrawType, "BOX"))
        _set(brush, "tint_color", _slate_color(color))
        _set(item_style, brush_name, brush)
    _set(item_style, "text_color", _slate_color(INK))
    _set(item_style, "selected_text_color", _slate_color(INK))
    _set(mode_combo, "item_style", item_style)
    _add(combo_frame, mode_combo)

    spacer = _construct(tree, unreal.Spacer, "TitleFlexibleSpacer")
    spacer_slot = content.add_child_to_vertical_box(spacer)
    _fill_vertical_slot(spacer_slot)

    button_alpha = _border(
        tree,
        "DepartureButtonAlphaFrame",
        unreal.LinearColor(0.73, 0.48, 0.13, 0.32),
        _margin(5, 5, 5, 5),
    )
    button_alpha_slot = content.add_child_to_vertical_box(button_alpha)
    _set_padding(button_alpha_slot, _margin(210, 12, 210, 12))

    button_frame = _border(
        tree,
        "DepartureButtonFrame",
        unreal.LinearColor(0.76, 0.53, 0.19, 0.90),
        _margin(2, 2, 2, 2),
    )
    _add(button_alpha, button_frame)

    departure_button = _construct(tree, unreal.Button, "DepartureButton")
    _call(departure_button, "set_background_color", unreal.LinearColor(0.14, 0.055, 0.025, 0.96))
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
        ):
            brush = button_style.get_editor_property(brush_name)
            _set(brush, "resource_object", departure_texture)
            _set(brush, "image_size", unreal.Vector2D(420, 96))
            _set(brush, "tint_color", _slate_color(tint))
            _set(button_style, brush_name, brush)
        _set(departure_button, "widget_style", button_style)
    _add(button_frame, departure_button)

    departure_label = _text(
        tree,
        "DepartureButtonText",
        "SET  SAIL",
        25,
        unreal.LinearColor(0.96, 0.81, 0.43, 1.0),
        2,
    )
    _set_padding(departure_label, _margin(18, 13, 18, 13))
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
    _set_padding(footer_slot, _margin(0, 16, 0, 0))

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
            "GraphicModeComboBox",
            "TitlePanel",
            "LogoImage",
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
