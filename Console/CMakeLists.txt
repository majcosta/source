add_library(Console INTERFACE)
target_sources(Console INTERFACE
"Console.cpp"
"Cursors.cpp"
"Dialogs.cpp"
"FileStream.cpp"
)
target_include_directories(Console INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
