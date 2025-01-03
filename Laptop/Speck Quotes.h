#ifndef _SPECK_QUOTES_H_
#define _SPECK_QUOTES_H_

//Enum,s for all of specks quotes
#ifdef JA2UB
enum
{
/*
Ja25: no longer in game
	SPECK_QUOTE_FIRST_TIME_IN_0,		//0
	SPECK_QUOTE_FIRST_TIME_IN_1,
	SPECK_QUOTE_FIRST_TIME_IN_2,
	SPECK_QUOTE_FIRST_TIME_IN_3,
	SPECK_QUOTE_FIRST_TIME_IN_4,
	SPECK_QUOTE_FIRST_TIME_IN_5,
	SPECK_QUOTE_FIRST_TIME_IN_6,
	SPECK_QUOTE_FIRST_TIME_IN_7,
	SPECK_QUOTE_FIRST_TIME_IN_8,
	SPECK_QUOTE_THANK_PLAYER_FOR_OPENING_ACCOUNT,

	SPECK_QUOTE_ALTERNATE_OPENING_1_TOUGH_START,						//10
	SPECK_QUOTE_ALTERNATE_OPENING_2_BUSINESS_BAD,
	SPECK_QUOTE_ALTERNATE_OPENING_3_BUSINESS_GOOD,
	SPECK_QUOTE_ALTERNATE_OPENING_4_TRYING_TO_RECRUIT,
	SPECK_QUOTE_ALTERNATE_OPENING_5_PLAYER_OWES_SPECK_ACCOUNT_SUSPENDED,
	SPECK_QUOTE_ALTERNATE_OPENING_6_PLAYER_OWES_SPECK_ALMOST_BANKRUPT_1,
	SPECK_QUOTE_ALTERNATE_OPENING_6_PLAYER_OWES_SPECK_ALMOST_BANKRUPT_2,
	SPECK_QUOTE_ALTERNATE_OPENING_7_PLAYER_ACCOUNT_OK,
	SPECK_QUOTE_ALTERNATE_OPENING_8_MADE_PARTIAL_PAYMENT,
	SPECK_QUOTE_ALTERNATE_OPENING_9_FIRST_VISIT_SINCE_SERVER_WENT_DOWN,

	SPECK_QUOTE_ALTERNATE_OPENING_10_GENERIC_OPENING,						//20
	SPECK_QUOTE_ALTERNATE_OPENING_10_TAG_FOR_20,
	SPECK_QUOTE_ALTERNATE_OPENING_11_NEW_MERCS_AVAILABLE,
*/
	SPECK_QUOTE_ALTERNATE_OPENING_12_PLAYERS_LOST_MERCS					=23,
/*
Ja25: no longer in game
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_PLAYER_OWES_MONEY,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_PLAYER_CLEARED_DEBT,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_PLAYER_CLEARED_OVERDUE_ACCOUNT,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_FIRST_MERC_DIES,
*/
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_BIFF_IS_DEAD							=28,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_HAYWIRE_IS_DEAD,

	SPECK_QUOTE_ALTERNATE_OPENING_TAG_GASKET_IS_DEAD,						//30
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_RAZOR_IS_DEAD,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_FLO_IS_DEAD_BIFF_ALIVE,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_FLO_IS_DEAD_BIFF_IS_DEAD,
/*
Ja25: no longer in game
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_FLO_MARRIED_A_COUSIN_BIFF_IS_ALIVE,
*/
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_GUMPY_IS_DEAD							=35,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_LARRY_IS_DEAD,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_LARRY_RELAPSED,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_COUGER_IS_DEAD,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_NUMB_IS_DEAD,

	SPECK_QUOTE_ALTERNATE_OPENING_TAG_BUBBA_IS_DEAD,						//40
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_ON_AFTER_OTHER_TAGS_1,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_ON_AFTER_OTHER_TAGS_2,
/*
Ja25: no longer in game
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_ON_AFTER_OTHER_TAGS_3,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_ON_AFTER_OTHER_TAGS_4,
	SPECK_QUOTE_PLAYER_MAKES_FULL_PAYMENT,
	SPECK_QUOTE_PLAYER_MAKES_PARTIAL_PAYMENT,
*/
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_BIFF						=47,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_HAYWIRE,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_GASKET,

	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_RAZOR,						//50
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_FLO,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_GUMPY,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_LARRY,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_COUGER,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_NUMB,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_BUBBA,
/*
Ja25: no longer in game
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_TAG_GETTING_MORE_MERCS,
*/
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_AIM_SLANDER_1=58,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_AIM_SLANDER_2,

	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_AIM_SLANDER_3,						//60
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_AIM_SLANDER_4,
	SPECK_QUOTE_BIFF_UNAVALIABLE,
	SPECK_QUOTE_PLAYERS_HIRES_BIFF_SPECK_PLUGS_LARRY,
	SPECK_QUOTE_PLAYERS_HIRES_BIFF_SPECK_PLUGS_FLO,
	SPECK_QUOTE_PLAYERS_HIRES_HAYWIRE_SPECK_PLUGS_RAZOR,
	SPECK_QUOTE_PLAYERS_HIRES_RAZOR_SPECK_PLUGS_HAYWIRE,
	SPECK_QUOTE_PLAYERS_HIRES_FLO_SPECK_PLUGS_BIFF,
	SPECK_QUOTE_PLAYERS_HIRES_LARRY_SPECK_PLUGS_BIFF,
	SPECK_QUOTE_GENERIC_THANKS_FOR_HIRING_MERCS_1,

	SPECK_QUOTE_GENERIC_THANKS_FOR_HIRING_MERCS_2,						//70
	SPECK_QUOTE_PLAYER_TRIES_TO_HIRE_ALREADY_HIRED_MERC,
	SPECK_QUOTE_GOOD_BYE_1,
	SPECK_QUOTE_GOOD_BYE_2,
	SPECK_QUOTE_GOOD_BYE_3,
/*
Ja25: no longer in game
	SPECK_QUOTE_GOOD_BYE_TAG_PLAYER_OWES_MONEY,
*/

//New Quotes

	SPECK_QUOTE_NEW_INTRO_1=76,
	SPECK_QUOTE_NEW_INTRO_2,
	SPECK_QUOTE_NEW_INTRO_3,
	SPECK_QUOTE_NEW_INTRO_4,

	SPECK_QUOTE_NEW_INTRO_5,//80
	SPECK_QUOTE_NEW_INTRO_6,
	SPECK_QUOTE_NEW_INTRO_7,
	SPECK_QUOTE_DEFAULT_INTRO_HAVENT_HIRED_MERCS,
	SPECK_QUOTE_DEFAULT_INTRO_HAVE_HIRED_MERCS,
	SPECK_QUOTE_BETTER_STARTING_EQPMNT_TAG_ON,
	SPECK_QUOTE_PLAYER_CANT_AFFORD_TO_HIRE_MERC,
	SPECK_QUOTE_ENCOURAGE_SHOP_TAG_ON,
	SPECK_QUOTE_2ND_INTRO_LAPTOP_WORKING_AGAIN_1,
	SPECK_QUOTE_2ND_INTRO_LAPTOP_WORKING_AGAIN_2,

	SPECK_QUOTE_2ND_INTRO_LAPTOP_WORKING_AGAIN_3,		//90
	SPECK_QUOTE_2ND_INTRO_LAPTOP_WORKING_AGAIN_4,
	SPECK_QUOTE_2ND_INTRO_LAPTOP_WORKING_AGAIN_5,
	SPECK_QUOTE_2ND_INTRO_LAPTOP_WORKING_AGAIN_6,
	SPECK_QUOTE_ADVERTISE_GASTON,
	SPECK_QUOTE_ADVERTISE_STOGIE,
	SPECK_QUOTE_GASTON_DEAD,
	SPECK_QUOTE_STOGIE_DEAD,
	SPECK_QUOTE_PLAYER_HIRES_GASTON,
	SPECK_QUOTE_PLAYER_HIRES_STOGIE,

	SPECK_QUOTE_RANDOM_CHIT_CHAT_1,				//100
	SPECK_QUOTE_RANDOM_CHIT_CHAT_2,
	SPECK_QUOTE_RANDOM_CHIT_CHAT_3,
	SPECK_QUOTE_BIFF_DEAD_WHEN_IMPORTING,

/*
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	
	*/
};
#else
enum
{
	SPECK_QUOTE_FIRST_TIME_IN_0,		//0
	SPECK_QUOTE_FIRST_TIME_IN_1,
	SPECK_QUOTE_FIRST_TIME_IN_2,
	SPECK_QUOTE_FIRST_TIME_IN_3,
	SPECK_QUOTE_FIRST_TIME_IN_4,
	SPECK_QUOTE_FIRST_TIME_IN_5,
	SPECK_QUOTE_FIRST_TIME_IN_6,
	SPECK_QUOTE_FIRST_TIME_IN_7,
	SPECK_QUOTE_FIRST_TIME_IN_8,
	SPECK_QUOTE_THANK_PLAYER_FOR_OPENING_ACCOUNT,

	SPECK_QUOTE_ALTERNATE_OPENING_1_TOUGH_START,						//10
	SPECK_QUOTE_ALTERNATE_OPENING_2_BUSINESS_BAD,
	SPECK_QUOTE_ALTERNATE_OPENING_3_BUSINESS_GOOD,
	SPECK_QUOTE_ALTERNATE_OPENING_4_TRYING_TO_RECRUIT,
	SPECK_QUOTE_ALTERNATE_OPENING_5_PLAYER_OWES_SPECK_ACCOUNT_SUSPENDED,
	SPECK_QUOTE_ALTERNATE_OPENING_6_PLAYER_OWES_SPECK_ALMOST_BANKRUPT_1,
	SPECK_QUOTE_ALTERNATE_OPENING_6_PLAYER_OWES_SPECK_ALMOST_BANKRUPT_2,
	SPECK_QUOTE_ALTERNATE_OPENING_7_PLAYER_ACCOUNT_OK,
	SPECK_QUOTE_ALTERNATE_OPENING_8_MADE_PARTIAL_PAYMENT,
	SPECK_QUOTE_ALTERNATE_OPENING_9_FIRST_VISIT_SINCE_SERVER_WENT_DOWN,

	SPECK_QUOTE_ALTERNATE_OPENING_10_GENERIC_OPENING,						//20
	SPECK_QUOTE_ALTERNATE_OPENING_10_TAG_FOR_20,
	SPECK_QUOTE_ALTERNATE_OPENING_11_NEW_MERCS_AVAILABLE,
	SPECK_QUOTE_ALTERNATE_OPENING_12_PLAYERS_LOST_MERCS,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_PLAYER_OWES_MONEY,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_PLAYER_CLEARED_DEBT,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_PLAYER_CLEARED_OVERDUE_ACCOUNT,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_FIRST_MERC_DIES,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_BIFF_IS_DEAD,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_HAYWIRE_IS_DEAD,

	SPECK_QUOTE_ALTERNATE_OPENING_TAG_GASKET_IS_DEAD,						//30
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_RAZOR_IS_DEAD,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_FLO_IS_DEAD_BIFF_ALIVE,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_FLO_IS_DEAD_BIFF_IS_DEAD,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_FLO_MARRIED_A_COUSIN_BIFF_IS_ALIVE,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_GUMPY_IS_DEAD,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_LARRY_IS_DEAD,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_LARRY_RELAPSED,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_COUGER_IS_DEAD,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_NUMB_IS_DEAD,

	SPECK_QUOTE_ALTERNATE_OPENING_TAG_BUBBA_IS_DEAD,						//40
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_ON_AFTER_OTHER_TAGS_1,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_ON_AFTER_OTHER_TAGS_2,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_ON_AFTER_OTHER_TAGS_3,
	SPECK_QUOTE_ALTERNATE_OPENING_TAG_ON_AFTER_OTHER_TAGS_4,
	SPECK_QUOTE_PLAYER_MAKES_FULL_PAYMENT,
	SPECK_QUOTE_PLAYER_MAKES_PARTIAL_PAYMENT,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_BIFF,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_HAYWIRE,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_GASKET,

	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_RAZOR,						//50
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_FLO,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_GUMPY,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_LARRY,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_COUGER,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_NUMB,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_BUBBA,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_TAG_GETTING_MORE_MERCS,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_AIM_SLANDER_1,
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_AIM_SLANDER_2,

	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_AIM_SLANDER_3,						//60
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_AIM_SLANDER_4,
	SPECK_QUOTE_BIFF_UNAVALIABLE,
	SPECK_QUOTE_PLAYERS_HIRES_BIFF_SPECK_PLUGS_LARRY,
	SPECK_QUOTE_PLAYERS_HIRES_BIFF_SPECK_PLUGS_FLO,
	SPECK_QUOTE_PLAYERS_HIRES_HAYWIRE_SPECK_PLUGS_RAZOR,
	SPECK_QUOTE_PLAYERS_HIRES_RAZOR_SPECK_PLUGS_HAYWIRE,
	SPECK_QUOTE_PLAYERS_HIRES_FLO_SPECK_PLUGS_BIFF,
	SPECK_QUOTE_PLAYERS_HIRES_LARRY_SPECK_PLUGS_BIFF,
	SPECK_QUOTE_GENERIC_THANKS_FOR_HIRING_MERCS_1,

	SPECK_QUOTE_GENERIC_THANKS_FOR_HIRING_MERCS_2,						//70
	SPECK_QUOTE_PLAYER_TRIES_TO_HIRE_ALREADY_HIRED_MERC,
	SPECK_QUOTE_GOOD_BYE_1,
	SPECK_QUOTE_GOOD_BYE_2,
	SPECK_QUOTE_GOOD_BYE_3,
	SPECK_QUOTE_GOOD_BYE_TAG_PLAYER_OWES_MONEY,

	//TODO: Madd - don't forget the sounds for these
	SPECK_QUOTE_ADVERTISE_GASTON, //76
	SPECK_QUOTE_ADVERTISE_STOGIE,
	SPECK_QUOTE_GASTON_DEAD,
	SPECK_QUOTE_STOGIE_DEAD,
	SPECK_QUOTE_PLAYER_HIRES_GASTON,
	SPECK_QUOTE_PLAYER_HIRES_STOGIE,

	SPECK_QUOTE_RANDOM_CHIT_CHAT_1,				
	SPECK_QUOTE_RANDOM_CHIT_CHAT_2,

	// anv: additional quotes for playable Speck
	SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_HIMSELF,
	SPECK_QUOTE_PLAYER_HIRES_SPECK,
	SPECK_QUOTE_PLAYER_HIRES_SPECK_TOGETHER_WITH_VICKI,
	SPECK_QUOTE_SPECK_UNAVAILABLE,
/*
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	SPECK_QUOTE_,

	SPECK_QUOTE_,						//80
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	SPECK_QUOTE_,
	SPECK_QUOTE_,
*/
};

// anv: quotes for Speck's recruitable alter ego
enum{

	SPECK_PLAYABLE_QUOTE_PLAYER_OWES_SPECK_ACCOUNT_SUSPENDED = 80,
	SPECK_PLAYABLE_QUOTE_PLAYER_OWES_SPECK_ALMOST_BANKRUPT_1,
	SPECK_PLAYABLE_QUOTE_PLAYER_OWES_SPECK_ALMOST_BANKRUPT_2,
	SPECK_PLAYABLE_QUOTE_NEW_MERCS_AVAILABLE =  83,
	SPECK_PLAYABLE_QUOTE_SERVER_WENT_DOWN = 84,

	SPECK_PLAYABLE_QUOTE_PLAYERS_LOST_MERCS = 85,

	SPECK_PLAYABLE_QUOTE_FIRST_MERC_DIES = 86,
	SPECK_PLAYABLE_QUOTE_BIFF_IS_DEAD,
	SPECK_PLAYABLE_QUOTE_HAYWIRE_IS_DEAD,

	SPECK_PLAYABLE_QUOTE_GASKET_IS_DEAD,
	SPECK_PLAYABLE_QUOTE_RAZOR_IS_DEAD,
	SPECK_PLAYABLE_QUOTE_FLO_IS_DEAD_BIFF_ALIVE,
	SPECK_PLAYABLE_QUOTE_FLO_IS_DEAD_BIFF_IS_DEAD,
	SPECK_PLAYABLE_QUOTE_GUMPY_IS_DEAD,
	SPECK_PLAYABLE_QUOTE_LARRY_IS_DEAD,	
	SPECK_PLAYABLE_QUOTE_COUGER_IS_DEAD,
	SPECK_PLAYABLE_QUOTE_NUMB_IS_DEAD,

	SPECK_PLAYABLE_QUOTE_BUBBA_IS_DEAD,
	SPECK_PLAYABLE_QUOTE_GASTON_DEAD,
	SPECK_PLAYABLE_QUOTE_STOGIE_DEAD,

	SPECK_PLAYABLE_QUOTE_LARRY_RELAPSED,
	SPECK_PLAYABLE_QUOTE_FLO_MARRIED_A_COUSIN_BIFF_IS_ALIVE,
};
#endif

#endif
