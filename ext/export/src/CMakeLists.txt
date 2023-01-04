option(JA2EXPORT "Build the Ja2 export tool.")

if(JA2EXPORT)
	message(STATUS "Configuring JA2EXPORT.")

	add_executable(JA2_EXPORT
		init_vfs.cpp
		main.cpp
		progress_bar.cpp
		ja2/himage.cpp
		ja2/XMLWriter.cpp
		export/jsd/export_jsd.cpp
		export/jsd/structure.cpp
		export/slf/export_slf.cpp
		export/sti/export_sti.cpp
		export/sti/Image.cpp
		export/sti/stci_image_utils.cpp
		export/sti/STCI_lib.cpp
	)
	target_include_directories(JA2_EXPORT PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/ja2"
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${CMAKE_SOURCE_DIR}/ext/VFS/include"
		"${CMAKE_SOURCE_DIR}/ext/libpng"
	)
	target_link_libraries(JA2_EXPORT PUBLIC bfVFS libpng)
else()
	message(STATUS "Not configuring JA2EXPORT by default.")
endif()

