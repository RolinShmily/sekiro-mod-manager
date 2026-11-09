#ifndef MINIZ_EXPORT_H
#define MINIZ_EXPORT_H

/*
 * Upstream miniz generates this header with CMake's generate_export_header() at configure
 * time, so it is absent from a raw source checkout. SMM compiles miniz into a STATIC library
 * (see third_party/CMakeLists.txt), which means there is no DLL boundary to export across and
 * the macro must expand to nothing.
 *
 * If this vendored copy is ever switched to a shared build, replace the definition below with
 * the generated one rather than leaving it empty: on MSVC an empty export macro for a shared
 * library silently drops every symbol.
 */
#ifndef MINIZ_EXPORT
#define MINIZ_EXPORT
#endif

#ifndef MINIZ_NO_EXPORT
#define MINIZ_NO_EXPORT
#endif

#endif /* MINIZ_EXPORT_H */
