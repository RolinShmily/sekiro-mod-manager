add_executable(smm
    src/main.cpp
    src/cli_parser.cpp
    src/json_output.cpp
    src/table.cpp
    src/terminal.cpp
    src/resolve.cpp
    src/commands/cmd_mods.cpp
    src/commands/cmd_deploy.cpp
    src/commands/cmd_import.cpp
    src/commands/cmd_doctor.cpp
    src/commands/cmd_presets.cpp
    src/commands/cmd_export.cpp
)

target_include_directories(smm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(smm PRIVATE SMM_VERSION_STRING="${PROJECT_VERSION}")
target_link_libraries(smm PRIVATE smm_core smm::warnings)
set_target_properties(smm PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

# Keep the CLI alias stable for build scripts.
add_executable(smm-cli ALIAS smm)
