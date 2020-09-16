file(GLOB_RECURSE ULOG_SOURCES
    ${PROJECT_SOURCE_DIR}/deps/ulog/*.h
    ${PROJECT_SOURCE_DIR}/deps/ulog/*.c
)

add_library(ulog
    SHARED
        ${ULOG_SOURCES}
)

target_link_libraries(ulog pthread)