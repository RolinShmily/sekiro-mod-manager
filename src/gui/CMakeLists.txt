cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------
# Both the GUI native client and the CLI link smm_core. Business logic stays in
# the core library; the GUI adapts its results into Qt models without subprocess IPC.
#
#   core  <-  cli
#         <-  gui
# ---------------------------------------------------------------------------

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network LinguistTools)

# Configure HuskarUI options
set(BUILD_HUSKARUI_GALLERY OFF CACHE BOOL "" FORCE)
set(BUILD_HUSKARUI_STATIC_LIBRARY ON CACHE BOOL "" FORCE)
set(BUILD_HUSKARUI_ON_DESKTOP_PLATFORM ON CACHE BOOL "" FORCE)
set(INSTALL_HUSKARUI_IN_DEFAULT_LOCATION OFF CACHE BOOL "" FORCE)

set(SMM_HUSKAR_ROOT ${CMAKE_SOURCE_DIR}/src/gui/third_party/HuskarUI)
if(NOT EXISTS ${SMM_HUSKAR_ROOT}/CMakeLists.txt)
    message(FATAL_ERROR
        "HuskarUI is missing. Fetch the submodule first:\n"
        "    git submodule update --init --recursive")
endif()

add_subdirectory(${SMM_HUSKAR_ROOT} ${CMAKE_BINARY_DIR}/huskarui EXCLUDE_FROM_ALL)

# ---------------------------------------------------------------------------
# Silence warnings on the vendor targets only.
#
# This used to be `set(CMAKE_CXX_FLAGS "... /wd4267 ...")`, which is a cache variable read at
# generation time for EVERY target: it silenced our own sources too and contradicted the
# project-wide /W4 policy. Applying `/w` (or -w) per target keeps third-party noise out while
# leaving smm_gui and the GUI tests under /W4.
#
# HuskarUI creates its targets in nested directories (3rdparty/qwindowkit, qml/...), so the
# directory tree is walked recursively; interface libraries are skipped because they cannot
# carry PRIVATE options.
# ---------------------------------------------------------------------------
function(smm_silence_vendor_warnings dir)
    get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target ${targets})
        get_target_property(target_type ${target} TYPE)
        # Only real compilable targets: HuskarUI also creates UTILITY targets (aotstats,
        # qmlls helpers) and INTERFACE libraries, neither of which accepts compile options.
        if(NOT target_type MATCHES "^(STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY|EXECUTABLE)$")
            continue()
        endif()
        if(MSVC)
            target_compile_options(${target} PRIVATE /w)
        else()
            target_compile_options(${target} PRIVATE -w)
        endif()
    endforeach()

    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirs})
        smm_silence_vendor_warnings("${subdir}")
    endforeach()
endfunction()

smm_silence_vendor_warnings("${SMM_HUSKAR_ROOT}")

qt_standard_project_setup(REQUIRES 6.5)

set(SMM_GUI_RC "")
if(WIN32)
    set(SMM_GUI_RC "${CMAKE_CURRENT_SOURCE_DIR}/resources/smm_gui.rc")
endif()

# Layering (see backend/):
#   controllers/  QML entry points, flow coordination, GUI state only
#   models/       QAbstractListModel projections; no filesystem access
#   services/     CoreService (sync core facade), TaskRunner (serialized bg work),
#                 UpdateFeed (pure update logic), UpdateService (network + UI strings)
#   adapters/     core value types <-> Qt value types, path encoding in one place
#   providers/    image resources for QML
qt_add_executable(smm_gui
    main.cpp
    backend/GuiTypes.hpp
    backend/adapters/CoreMapping.hpp
    backend/adapters/CoreMapping.cpp
    backend/services/TaskTypes.hpp
    backend/services/CoreService.hpp
    backend/services/CoreService.cpp
    backend/services/BackgroundTasks.hpp
    backend/services/BackgroundTasks.cpp
    backend/services/UpdateFeed.hpp
    backend/services/UpdateFeed.cpp
    backend/services/UpdateService.hpp
    backend/services/UpdateService.cpp
    backend/models/ModListModel.hpp
    backend/models/ModListModel.cpp
    backend/models/AssetListModel.hpp
    backend/models/AssetListModel.cpp
    backend/models/PresetListModel.hpp
    backend/models/PresetListModel.cpp
    backend/controllers/GuiController.hpp
    backend/controllers/GuiController.cpp
    backend/providers/ModPreviewImageProvider.hpp
    backend/providers/ModPreviewImageProvider.cpp
    ${SMM_GUI_RC}
)

qt_add_qml_module(smm_gui
    URI "SmmGui"
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES
        qml/Main.qml
        qml/views/LaunchHubView.qml
        qml/views/ArmouryView.qml
        qml/views/ModPackView.qml
        qml/views/SettingsView.qml
        qml/views/AboutView.qml
        qml/components/PageHeader.qml
        qml/components/SectionCard.qml
        qml/components/ModCard.qml
        qml/components/ModDrawer.qml
        qml/components/SavePackModal.qml
        qml/components/DoctorModal.qml
        qml/components/NavItem.qml
)

target_include_directories(smm_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SMM_HUSKAR_ROOT}/src/cpp
    ${SMM_HUSKAR_ROOT}/src_impl/cpp
)

# ---------------------------------------------------------------------------
# 国际化：.ts 源码 -> .qm 编译二进制 -> 嵌入资源系统 :/i18n/smm_en_US.qm
# ---------------------------------------------------------------------------
set(SMM_QM "${CMAKE_CURRENT_BINARY_DIR}/smm_en_US.qm")
add_custom_command(
    OUTPUT "${SMM_QM}"
    COMMAND Qt6::lrelease "${CMAKE_CURRENT_SOURCE_DIR}/translations/smm_en_US.ts" -qm "${SMM_QM}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/translations/smm_en_US.ts"
    COMMENT "Compiling translations/smm_en_US.ts -> smm_en_US.qm"
    VERBATIM
)
qt_add_resources(smm_gui "smm_translations"
    PREFIX "/i18n"
    BASE "${CMAKE_CURRENT_BINARY_DIR}"
    FILES "${SMM_QM}"
)

qt_add_resources(smm_gui "smm_images"
    PREFIX "/images"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources/images"
    FILES
        resources/images/sekiro_seal_32.png
        resources/images/sekiro_seal_64.png
        resources/images/sekiro_seal_128.png
        resources/images/sekiro_seal_256.png
        resources/images/sekiro_icon_32.png
        resources/images/sekiro_icon_64.png
        resources/images/sekiro_icon_128.png
        resources/images/sekiro_icon_256.png
        resources/images/sekiro_calligraphy_white.png
        resources/images/sekiro_calligraphy_crimson.png
        resources/images/sekiro_calligraphy_gold.png
        resources/images/sekiro_hero_art.png
        resources/images/icon_nexus.png
        resources/images/icon_banana.png
        resources/images/icon_3dm.png
        resources/images/icon_nexus.svg
        resources/images/icon_banana.svg
        resources/images/icon_3dm.svg
        resources/images/icon_bilibili.svg
        resources/images/icon_github.svg
)

target_compile_definitions(smm_gui PRIVATE
    BUILD_HUSKARUI_STATIC_LIBRARY
    SMM_VERSION_STRING="${PROJECT_VERSION}"
    SMM_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    SMM_BUILD_DIR="${CMAKE_BINARY_DIR}"
)

target_link_libraries(smm_gui PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    HuskarUIBasic
    $<$<BOOL:${BUILD_HUSKARUI_STATIC_LIBRARY}>:HuskarUIImplPlugin>
    $<$<BOOL:${BUILD_HUSKARUI_STATIC_LIBRARY}>:HuskarUIBasicPlugin>
    smm::warnings
    smm_core
)

set_target_properties(smm_gui PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    WIN32_EXECUTABLE $<CONFIG:Release>
)

add_executable(smm-gui ALIAS smm_gui)

# ---------------------------------------------------------------------------
# GUI-layer unit tests that need no QML engine and no window: the pure update feed and the
# CoreService adapter. Both are headless, so neither needs HuskarUI nor a display.
# ---------------------------------------------------------------------------
if(SMM_BUILD_TESTS)
    add_executable(smm_update_feed_tests
        tests/update_feed_tests.cpp
        backend/services/UpdateFeed.cpp
    )
    target_include_directories(smm_update_feed_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(smm_update_feed_tests PRIVATE smm_core smm::warnings)
    add_test(NAME smm_update_feed_tests COMMAND smm_update_feed_tests)

    # Headless adapter test: CoreService + CoreMapping, Qt Core only. No window, no HuskarUI.
    add_executable(smm_core_service_tests
        tests/core_service_tests.cpp
        backend/adapters/CoreMapping.cpp
        backend/services/CoreService.cpp
    )
    target_include_directories(smm_core_service_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(smm_core_service_tests PRIVATE Qt6::Core smm_core smm::warnings)
    # The binary links Qt6::Core, so the Qt runtime directory must be on PATH for ctest: the
    # build tree is not deployed and Windows resolves the test process' DLLs from PATH.
    if(WIN32)
        add_test(NAME smm_core_service_tests
            COMMAND ${CMAKE_COMMAND} -E env "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
                    $<TARGET_FILE:smm_core_service_tests>)
    else()
        add_test(NAME smm_core_service_tests COMMAND smm_core_service_tests)
    endif()
endif()
