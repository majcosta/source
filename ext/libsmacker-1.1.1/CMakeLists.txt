add_library(libsmacker
smacker.c
driver.c
smk_hufftree.c
smk2avi.c
smk_bitstream.c
)
target_include_directories(libsmacker PUBLIC
${CMAKE_CURRENT_SOURCE_DIR}
)
