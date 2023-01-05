#include "Language Defines.h"

constexpr auto Language() -> std::string_view {
#ifdef ENGLISH
#pragma message(" (Language set to ENGLISH, You'll need english CDs)")
	return "ENGLISH";
#elif CHINESE
#pragma message(" (Language set to CHINESE, You'll need chinese CDs)")
	return "CHINESE";
#elif DUTCH
#pragma message(" (Language set to DUTCH, You'll need dutch CDs)")
	return "DUTCH";
#elif ITALIAN
#pragma message(" (Language set to ITALIAN, You'll need italian CDs)")
	return "ITALIAN";
#elif RUSSIAN
#pragma message(" (Language set to RUSSIAN, You'll need russian CDs)")
	return "RUSSIAN";
#elif GERMAN
#pragma message(" (Language set to GERMAN, You'll need Topware/german CDs)")
	return  "GERMAN";
#elif FRENCH
#pragma message(" (Language set to FRENCH, You'll need french CDs)")
	return  "FRENCH";
#elif POLISH
#pragma message(" (Language set to POLISH, You'll need polish CDs)")
	return "POLISH";
#endif
}

// Tests
#ifdef ENGLISH
static_assert(Language() == "ENGLISH", "lang() must be able to be evaluated at compile-time");
static_assert(Language().substr(0,2) == "EN", "lang().substr(...) must be able to be evaluated at compile-time");
#endif
