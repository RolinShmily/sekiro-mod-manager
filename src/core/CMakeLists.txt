add_subdirectory(third_party)

add_library(smm_core STATIC
    error.cpp
    types.cpp
    normalizer.cpp
    loader.cpp
    manager.cpp
    conflict.cpp
    deploy.cpp
    executor.cpp
    extractor.cpp
    importer.cpp
    exporter.cpp
    doctor.cpp
    preset.cpp
)

# Consumers include headers as "core/<header>.hpp"; internal sources include "<header>.hpp".
target_include_directories(smm_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# smm_json is PUBLIC because core/types.hpp exposes nlohmann::json in its declarations;
# smm_miniz stays PRIVATE since no public header mentions zip.
target_link_libraries(smm_core
    PUBLIC smm_json
    PRIVATE smm_miniz smm::warnings
)

# Windows-only APIs (CreateHardLinkW, GetFileInformationByHandle, volume serial numbers,
# Steam library discovery via the registry) live behind _WIN32 guards in the sources.
if(WIN32)
    target_link_libraries(smm_core PRIVATE advapi32 shell32 ole32)
endif()

set_target_properties(smm_core PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
