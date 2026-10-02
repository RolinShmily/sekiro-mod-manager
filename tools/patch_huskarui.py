#!/usr/bin/env python3
"""
Automated patch script for third_party/HuskarUI.
Ensures HuskarUI builds reliably on standard Qt 6 installations without ShaderTools,
and includes crash fixes for HusSegmented and button tactile transitions.
"""

import os
import sys

def patch_file(filepath, replacements):
    if not os.path.exists(filepath):
        print(f"[patch_huskarui] Warning: {filepath} not found.")
        return False
    
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    modified = False
    for old_txt, new_txt in replacements:
        if old_txt in content:
            content = content.replace(old_txt, new_txt)
            modified = True

    if modified:
        with open(filepath, "w", encoding="utf-8", newline="\n") as f:
            f.write(content)
        print(f"[patch_huskarui] Applied patch to: {filepath}")
        return True
    return False

def main():
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    huskarui_dir = os.path.join(repo_root, "third_party", "HuskarUI")
    if not os.path.exists(huskarui_dir):
        print("[patch_huskarui] third_party/HuskarUI not found, skipping.")
        return 0

    # 1. Patch src/CMakeLists.txt: remove required ShaderTools & conditionally wrap qt_add_shaders
    cmake_path = os.path.join(huskarui_dir, "src", "CMakeLists.txt")
    patch_file(cmake_path, [
        (
            "find_package(Qt6 6.5 COMPONENTS Quick QuickTemplates2 LabsQmlModels ShaderTools REQUIRED)",
            "find_package(Qt6 6.5 COMPONENTS Quick QuickTemplates2 LabsQmlModels REQUIRED)"
        ),
        (
            """qt_add_shaders(${PROJECT_NAME} "shaders"
    PREFIX "/HuskarUI"
    OUTPUT_TARGETS HUSKARUI_SHADERS_OUTPUT_TARGETS #Export for static lib
    FILES
        shaders/husrate.frag
        shaders/husliquidglass.vert
        shaders/husliquidglass.frag
)""",
            """if(COMMAND qt_add_shaders)
    qt_add_shaders(${PROJECT_NAME} "shaders"
        PREFIX "/HuskarUI"
        OUTPUT_TARGETS HUSKARUI_SHADERS_OUTPUT_TARGETS #Export for static lib
        FILES
            shaders/husrate.frag
            shaders/husliquidglass.vert
            shaders/husliquidglass.frag
    )
else()
    list(FILTER QML_SOURCES EXCLUDE REGEX "HusRate\\\\.qml|HusLiquidGlass\\\\.qml")
endif()"""
        )
    ])

    # 2. Patch src/imports/HusSegmented.qml: prevent div-by-zero crash and itemDelegate layout bug
    seg_path = os.path.join(huskarui_dir, "src", "imports", "HusSegmented.qml")
    patch_file(seg_path, [
        (
            "implicitWidth: block ? parent.width : Math.max(implicitBackgroundWidth + leftInset + rightInset,",
            "implicitWidth: (block && parent && parent.width > 0) ? parent.width : Math.max(implicitBackgroundWidth + leftInset + rightInset,"
        ),
        (
            "return ((__contentItem.width - __listModel.count * __listView.spacing) / __listModel.count );",
            "return __listModel.count > 0 ? Math.max(0, (__contentItem.width - __listModel.count * __listView.spacing) / __listModel.count) : 0;"
        ),
        (
            "return ((__contentItem.height - __listModel.count * __listView.spacing) / __listModel.count );",
            "return __listModel.count > 0 ? Math.max(0, (__contentItem.height - __listModel.count * __listView.spacing) / __listModel.count) : 0;"
        ),
        (
            """    property Component itemDelegate: Item {
        id: __itemDelegate
        width: __row.implicitWidth + (control.orientation === Qt.Horizontal ? 20 * control.sizeRatio : 0)
        height: __row.implicitHeight + (control.orientation === Qt.Horizontal ? 0 : 8 * control.sizeRatio)""",
            """    property Component itemDelegate: Item {
        id: __itemDelegate
        implicitWidth: __row.implicitWidth + (control.orientation === Qt.Horizontal ? 20 * control.sizeRatio : 0)
        implicitHeight: __row.implicitHeight + (control.orientation === Qt.Horizontal ? 0 : 8 * control.sizeRatio)
        width: implicitWidth
        height: implicitHeight"""
        ),
        (
            """            delegate: HusRectangleInternal {
                id: __rootItem
                implicitWidth: {""",
            """            delegate: HusRectangleInternal {
                id: __rootItem
                width: implicitWidth
                height: implicitHeight
                implicitWidth: {"""
        )
    ])

    # 3. Patch src/imports/HusButton.qml: tactile animation
    btn_path = os.path.join(huskarui_dir, "src", "imports", "HusButton.qml")
    patch_file(btn_path, [
        (
            """    property int hoverCursorShape: Qt.PointingHandCursor
    property int type: HusButton.Type_Default
    property int shape: HusButton.Shape_Default

    property color colorText: {""",
            """    property int hoverCursorShape: Qt.PointingHandCursor
    property int type: HusButton.Type_Default
    property int shape: HusButton.Shape_Default

    focusPolicy: Qt.TabFocus
    scale: (control.enabled && control.down) ? 0.985 : 1.0
    opacity: (control.enabled && control.down) ? 0.86 : 1.0

    Behavior on scale {
        enabled: control.animationEnabled
        NumberAnimation { duration: 75; easing.type: Easing.OutQuad }
    }
    Behavior on opacity {
        enabled: control.animationEnabled
        NumberAnimation { duration: 75; easing.type: Easing.OutQuad }
    }

    property color colorText: {"""
        ),
        (
            "__effect.border.width = 8;",
            "__effect.border.width = 2;"
        )
    ])

    print("[patch_huskarui] Verification and patching completed.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
