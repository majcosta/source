add_library(ModularizedTacticalAI INTERFACE)
target_sources(ModularizedTacticalAI INTERFACE
"src/LegacyArmedVehiclePlan.cpp"
"src/AbstractPlanFactory.cpp"
"src/CrowPlan.cpp"
"src/LegacyAIPlan.cpp"
"src/LegacyAIPlanFactory.cpp"
"src/LegacyCreaturePlan.cpp"
"src/LegacyZombiePlan.cpp"
"src/NullPlan.cpp"
"src/NullPlanFactory.cpp"
"src/Plan.cpp"
"src/PlanFactoryLibrary.cpp"
"src/PlanList.cpp"
)
target_include_directories(ModularizedTacticalAI INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
