add_library(smm_core STATIC
    src/error.cpp
    src/types.cpp
    src/normalizer.cpp
    src/loader.cpp
    src/manager.cpp
    src/conflict.cpp
    src/deploy.cpp
    src/executor.cpp
    src/extractor.cpp
    src/importer.cpp
    src/exporter.cpp
    src/doctor.cpp
    src/runtime_doctor.cpp
    src/preset.cpp
)

target_include_directories(smm_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# smm_json is PUBLIC because smm/types.hpp exposes nlohmann::json in its declarations;
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
