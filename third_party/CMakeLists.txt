# Vendored third-party code. Deliberately minimal: the GUI's UI toolkit (ElaWidgetTools) is a
# git submodule consumed by src/gui/CMakeLists.txt, not from here.

# ---------------------------------------------------------------------------
# nlohmann/json - header-only JSON.
#
# Exposed as an INTERFACE target with a PUBLIC include path because smm_core's own public
# headers include <nlohmann/json.hpp>: every consumer of the core library needs this include
# directory to compile at all, so it cannot stay private to the core target.
# ---------------------------------------------------------------------------
add_library(smm_json INTERFACE)
target_include_directories(smm_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
add_library(smm::json ALIAS smm_json)

# ---------------------------------------------------------------------------
# miniz - zip container reading/writing for mod import and modpack export.
# Public domain / MIT (see third_party/miniz/LICENSE).
# ---------------------------------------------------------------------------
add_library(smm_miniz STATIC
    miniz/miniz.c
    miniz/miniz_tdef.c
    miniz/miniz_tinfl.c
    miniz/miniz_zip.c
)
target_include_directories(smm_miniz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/miniz)
set_target_properties(smm_miniz PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    # Third-party C code: silence the /W4 deluge without touching the sources.
    target_compile_options(smm_miniz PRIVATE /W0 /wd4996)
    target_compile_definitions(smm_miniz PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(smm_miniz PRIVATE -w)
endif()
