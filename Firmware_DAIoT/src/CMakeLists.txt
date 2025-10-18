# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
    SRCS ${app_sources}
    EMBED_TXTFILES
        ../data/html_page_index.html
        ../data/html_page_wifi_config.html
        ../data/html_page_device_data.html
    EMBED_FILES
        ../cert/client.crt
        ../cert/client.key
        ../cert/ca.crt
)
