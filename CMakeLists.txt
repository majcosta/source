cmake_minimum_required(VERSION 3.24)

project(ja2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# lua51.lib and lua51.vc9.lib have been built with /MTx, so we must as well
# TODO: maybe build our own Lua of the same version and use /MDx instead
add_compile_options($<IF:$<CONFIG:Debug>,/MTd,/MT>)

add_compile_definitions(CINTERFACE XML_STATIC VFS_STATIC VFS_WITH_SLF VFS_WITH_7ZIP USE_VFS _CRT_SECURE_NO_DEPRECATE)
include_directories(${CMAKE_SOURCE_DIR} Utils TileEngine TacticalAI ModularizedTacticalAI Tactical Strategic "Standard Gaming Platform" Res Lua Laptop Multiplayer "Multiplayer/raknet" Editor Console "ext/VFS/include")

add_subdirectory("ext/libpng")
add_subdirectory("ext/zlib")
add_subdirectory("ext/VFS")
target_link_libraries(bfVFS PRIVATE 7z)

add_subdirectory("TileEngine")
add_subdirectory("TacticalAI")
add_subdirectory("Utils")
add_subdirectory("Strategic")
add_subdirectory("Standard Gaming Platform")
add_subdirectory("lua")
add_subdirectory("Laptop")
add_subdirectory("Editor")
add_subdirectory("Console")
add_subdirectory("Tactical")
add_subdirectory("ModularizedTacticalAI")

set(Ja2_Source_Files
"aniviewscreen.cpp"
"Credits.cpp"
"Fade Screen.cpp"
"FeaturesScreen.cpp"
"GameInitOptionsScreen.cpp"
"gameloop.cpp"
"gamescreen.cpp"
"GameSettings.cpp"
"GameVersion.cpp"
"HelpScreen.cpp"
"Init.cpp"
"Intro.cpp"
"JA2 Splash.cpp"
"Ja25Update.cpp"
"jascreens.cpp"
"Language Defines.cpp"
"Loading Screen.cpp"
"MainMenuScreen.cpp"
"MessageBoxScreen.cpp"
"MPChatScreen.cpp"
"MPConnectScreen.cpp"
"MPHostScreen.cpp"
"MPJoinScreen.cpp"
"MPScoreScreen.cpp"
"MPXmlTeams.cpp"
"Multiplayer/client.cpp"
"Multiplayer/server.cpp"
"Multiplayer/transfer_rules.cpp"
"Options Screen.cpp"
"profiler.cpp"
"SaveLoadGame.cpp"
"SaveLoadScreen.cpp"
"SCREENS.cpp"
"Sys Globals.cpp"
"ub_config.cpp"
"XML_DifficultySettings.cpp"
"XML_IntroFiles.cpp"
"XML_Layout_MainMenu.cpp"
Res/ja2.rc
)

set(Ja2_Libraries
"${PROJECT_SOURCE_DIR}/libexpatMT.lib"
"Dbghelp.lib"
SGP
Laptop
Tactical
Strategic
Lua
TileEngine
libpng
zlib
Utils
"${PROJECT_SOURCE_DIR}/lua51.lib"
"${PROJECT_SOURCE_DIR}/lua51.vc9.lib"
"Winmm.lib"
"${PROJECT_SOURCE_DIR}/SMACKW32.LIB"
"${PROJECT_SOURCE_DIR}/binkw32.lib"
bfVFS
"${PROJECT_SOURCE_DIR}/Multiplayer/raknet/RakNetLibStatic.lib"
"ws2_32.lib"
TacticalAI
ModularizedTacticalAI
Editor
)

include(cmake/ValidateOptions.cmake)

set(ValidLanguages CHINESE DUTCH ENGLISH FRENCH GERMAN ITALIAN POLISH RUSSIAN)
ValidateOptions("${ValidLanguages}" "Languages" "ENGLISH" "${Languages}" "LangTargets")

set(ValidApplications JA2 JA2MAPEDITOR JA2UB JA2UBMAPEDITOR)
ValidateOptions("${ValidApplications}" "Applications" "JA2" "${Applications}" "ApplicationTargets")

foreach(lang IN LISTS LangTargets)
	foreach(exe IN LISTS ApplicationTargets)
		set(targName ${exe}_${lang})
		add_executable(${targName} WIN32 ${Ja2_Source_Files})
		target_link_libraries(${targName} PUBLIC ${Ja2_Libraries})

		set(isEditor $<STREQUAL:${exe},JA2MAPEDITOR>)
		set(isUb $<STREQUAL:${exe},JA2UB>)
		set(isUbEditor $<STREQUAL:${exe},JA2UBMAPEDITOR>)
		target_compile_definitions(${targName} PUBLIC
			$<IF:${isEditor},JA2EDITOR;JA2BETAVERSION,>
			$<IF:${isUb},JA2UB;JA2UBMAPS,>
			$<IF:${isUbEditor},JA2UB;JA2UBMAPS;JA2EDITOR;JA2BETAVERSION,>
			$<IF:$<CONFIG:Debug>,JA2BETAVERSION;JA2TESTVERSION;DEBUG_ATTACKBUSY,>
			${lang}
		)
	endforeach()
endforeach()
