add_library(TacticalAI INTERFACE)
target_sources(TacticalAI INTERFACE
"AIList.cpp"
"AIMain.cpp"
"AIUtils.cpp"
"Attacks.cpp"
"CreatureDecideAction.cpp"
"DecideAction.cpp"
"FindLocations.cpp"
"Knowledge.cpp"
"Medical.cpp"
"Movement.cpp"
"NPC.cpp"
"PanicButtons.cpp"
"QuestDebug.cpp"
"Realtime.cpp"
"ZombieDecideAction.cpp"
)
target_include_directories(TacticalAI INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
