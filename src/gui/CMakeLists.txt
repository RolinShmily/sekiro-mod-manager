cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------
# The GUI never links smm_core. Every piece of business logic is reached by spawning the `smm`
# CLI and reading its JSON. That is what keeps this layer a pure renderer, and it is why this
# file contains no reference to the core library at all.
#
#   core  <-  cli  <-  gui
# ---------------------------------------------------------------------------

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network LinguistTools)

# Configure HuskarUI options
set(BUILD_HUSKARUI_GALLERY OFF CACHE BOOL "" FORCE)
set(BUILD_HUSKARUI_STATIC_LIBRARY ON CACHE BOOL "" FORCE)
set(BUILD_HUSKARUI_ON_DESKTOP_PLATFORM ON CACHE BOOL "" FORCE)
set(INSTALL_HUSKARUI_IN_DEFAULT_LOCATION OFF CACHE BOOL "" FORCE)

set(SMM_HUSKAR_ROOT ${CMAKE_SOURCE_DIR}/third_party/HuskarUI)
if(NOT EXISTS ${SMM_HUSKAR_ROOT}/CMakeLists.txt)
    message(FATAL_ERROR
        "HuskarUI is missing. Fetch the submodule first:\n"
        "    git submodule update --init --recursive")
endif()

# Suppress vendor warnings on HuskarUI
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267 /wd4244 /wd4100")
endif()

add_subdirectory(${SMM_HUSKAR_ROOT} ${CMAKE_BINARY_DIR}/huskarui EXCLUDE_FROM_ALL)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(smm_gui
    main.cpp
    backend/SmmClient.hpp
    backend/SmmClient.cpp
    backend/Models.hpp
    backend/Models.cpp
    backend/UpdateManager.hpp
    backend/UpdateManager.cpp
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
        qml/components/ModCard.qml
        qml/components/ModDrawer.qml
        qml/components/SettingsModal.qml
        qml/components/SavePackModal.qml
        qml/components/AboutModal.qml
        qml/components/DoctorModal.qml
        qml/components/UpdateModal.qml
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
        resources/images/sekiro_silvergrass.png
        resources/images/sekiro_mortal_blade.png
)

qt_add_resources(smm_gui "smm_fonts"
    PREFIX "/fonts"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts"
    FILES
        resources/fonts/Inter.ttf
        resources/fonts/JetBrainsMono.ttf
        resources/fonts/InstrumentSerif-Regular.ttf
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
