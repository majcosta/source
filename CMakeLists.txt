cmake_minimum_required(VERSION 3.24)

project(ja2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# lua51.lib and lua51.vc9.lib have been built with /MTx, so we must as well
# TODO: build our own Lua 5.1.2 from source so we can use whichever
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_compile_definitions(CINTERFACE XML_STATIC VFS_STATIC VFS_WITH_SLF VFS_WITH_7ZIP USE_VFS _CRT_SECURE_NO_DEPRECATE)
include_directories(Ja2 "ext/VFS/include" Utils TileEngine TacticalAI "ModularizedTacticalAI/include" Tactical Strategic SGP "Ja2/Res" Lua Laptop Multiplayer Editor Console)

# external libraries
add_subdirectory("ext/libpng")
add_subdirectory("ext/zlib")
add_subdirectory("ext/VFS")
target_link_libraries(bfVFS PRIVATE 7z)

# ja2export utility
add_subdirectory("ext/export/src")

# internal libraries that are Good
add_subdirectory(Lua)
add_subdirectory(Multiplayer)

# internal libraries that live in Preprocessor Hell, because they are Bad
set(Ja2_Libs
TileEngine
TacticalAI
Utils
Strategic
SGP
Laptop
Editor
Console
Tactical
ModularizedTacticalAI
)
foreach(lib IN LISTS Ja2_Libs)
	add_subdirectory(${lib})
endforeach()

add_subdirectory(Ja2)

include(cmake/ValidateOptions.cmake)

set(ValidLanguages CHINESE DUTCH ENGLISH FRENCH GERMAN ITALIAN POLISH RUSSIAN)
ValidateOptions("${ValidLanguages}" "Languages" "ENGLISH" "${Languages}" "LangTargets")

set(ValidApplications JA2 JA2MAPEDITOR JA2UB JA2UBMAPEDITOR)
ValidateOptions("${ValidApplications}" "Applications" "JA2" "${Applications}" "ApplicationTargets")


# Due to widespread preprocessor definition abuse in the codebase, practically
# every library-language-executable combination is its own compilation target
# TODO: refactor preprocessor usage onto, ideally, a single translation unit
foreach(lang IN LISTS LangTargets)
	foreach(exe IN LISTS ApplicationTargets)
		set(targPrefix ${exe}_${lang})

		# make a copy of the library list for each language/library combination
		list(SUBLIST Ja2_Libs 0 -1 ${targPrefix}_Targets)

		add_executable(${targPrefix} WIN32)
		target_sources(${targPrefix} PRIVATE ${Ja2Src})
		target_link_libraries(${targPrefix} PRIVATE ${Ja2_Libraries})

		foreach(lib IN LISTS ${targPrefix}_Targets)
			set(tgt ${targPrefix}_${lib})

			add_library(${tgt})
			target_sources(${tgt} PRIVATE ${${lib}Src})

			set(isEditor $<STREQUAL:${exe},JA2MAPEDITOR>)
			set(isUb $<STREQUAL:${exe},JA2UB>)
			set(isUbEditor $<STREQUAL:${exe},JA2UBMAPEDITOR>)

			target_compile_definitions(${tgt} PUBLIC
				$<IF:${isEditor},JA2EDITOR;JA2BETAVERSION,>
				$<IF:${isUb},JA2UB;JA2UBMAPS,>
				$<IF:${isUbEditor},JA2UB;JA2UBMAPS;JA2EDITOR;JA2BETAVERSION,>
				$<IF:$<CONFIG:Debug>,JA2BETAVERSION;JA2TESTVERSION;DEBUG_ATTACKBUSY,>
				${lang}
			)
			target_link_libraries(${targPrefix} PUBLIC ${tgt})
		endforeach()

		# SGP is the only one calling these, so they can go here
		target_link_libraries(${targPrefix}_SGP PRIVATE "ddraw.lib" "${PROJECT_SOURCE_DIR}/fmodvc.lib")
		target_link_libraries(${targPrefix}_SGP PUBLIC libpng)
	endforeach()
endforeach()
