set (BUILD_TESTING OFF)
set (WITH_STL 1)
set (WITH_OTLP 1)
set (WITH_OTLP_GRPC 0)
set (WITH_OTLP_HTTP 1)

set (CMAKE_POLICY_DEFAULT_CMP0077 NEW)
add_subdirectory(${TiFlash_SOURCE_DIR}/contrib/opentelemetry ${CMAKE_CURRENT_BINARY_DIR}/opentelemetry)
unset (CMAKE_POLICY_DEFAULT_CMP0077)

