#pragma once

#include <unordered_map>

namespace kbf {

    enum KnownSituation {
        isOnline						  = 1,     //Online                      - Player is not Offline or Solo - Online   
        isSoloOnline					  = 2,     //SoloOnline                  - Exclusive to the Solo - Online mode
        isOfflineorMainMenu				  = 3,     //Offline                     - Game is in Offline mode, also true while in the main menu
        isinQuestPreparing				  = 4,     //QuestPreparing              - Player has accepted a Quest but has chosen to "Prepare"
        isinQuestReady					  = 5,     //QuestReady                  - Player has accepted a Quest and has chosen "Ready"
        isinQuestPlayingasHost			  = 6,     //QuestPlayingHost            - In Quest, playing as Host(not Arena)
        isinQuestPlayingasGuest			  = 7,     //QuestPlayingGuest           - In Quest, playing as Guest(not Arena)
        isinQuestPlayingfromFieldSurvey	  = 8,     // (? ? ? )                   - In Quest, when the quest was started as a Field Survey
        DUPLICATE_isinQuestPlayingasGuest = 9,     // (? ? ? )                   - In Quest, playing as Guest(not Arena)
        isinArenaQuestPlayingasHost		  = 10,    //DeclarationQuestPlayingHost - The Quest has started and the Player is the Host
        isinQuestPressSelectToEnd		  = 16,    //QuestHagitori               - End Quest UI prompt active
        isinQuestEndAnnounce			  = 17,    //QuestEndAnnounce            - The "Quest Complete" / "Quest Abandonned" / "Quest Failed" small cutscene
        isinQuestResultScreen			  = 18,    //QuestResult                 - Viewing Quest results screen
        isinQuestLoadingResult			  = 29,    //Lv2 missions                - Loading that happens before results
        isinLinkPartyAsGuest			  = 35,    //LinkPartyGuest              - The player is in a link party not as the host
        isinTrainingArea				  = 36,    // (? ? ? )                   - Player is in training area
        isinJunctionArea				  = 37,    //JunctionArea                - Special transition or linking zone(example: Suja < ->Wyveria passage)
        isinSuja						  = 38,    //ST503                       - Player is in the town of Suja
        isinGrandHub					  = 39,    //ST404                       - Player is in the GrandHub
        DUPLICATE_isinTrainingArea		  = 40,    //Arena                       - Second check for the Training area
        isinBowlingGame					  = 41,    //PlayingBowlingGame          - True once the player confirms with the NPC that they wish to start a Bowling mini - game
        isinArmWrestling				  = 42,    //PlayingArmWrestling         - True as soon as the player sits at a Arm wrestling table
        isatTable						  = 43,    //TableAction                 - Sitting at gathering hub table
        isAlwaysOn						  = 44,    // (? ? ? )                   - Don't know what it does, seemly always on while in session
    };

    const std::unordered_map<size_t, const char*> SITUATION_NAMES = {
        { 1, "isOnline" },
        { 2, "isSoloOnline" },
        { 3, "isOfflineorMainMenu" },
        { 4, "isinQuestPreparing" },
        { 5, "isinQuestReady" },
        { 6, "isinQuestPlayingasHost" },
        { 7, "isinQuestPlayingasGuest" },
        { 8, "isinQuestPlayingfromFieldSurvey" },
        { 9, "DUPLICATE_isinQuestPlayingasGuest" },
        { 10, "isinArenaQuestPlayingasHost" },
        { 16, "isinQuestPressSelectToEnd" },
        { 17, "isinQuestEndAnnounce" },
        { 18, "isinQuestResultScreen" },
        { 29, "isinQuestLoadingResult" },
        { 35, "isinLinkPartyAsGuest" },
        { 36, "isinTrainingArea" },
        { 37, "isinJunctionArea" },
        { 38, "isinSuja" },
        { 39, "isinGrandHub" },
        { 40, "DUPLICATE_isinTrainingArea" },
        { 41, "isinBowlingGame" },
        { 42, "isinArmWrestling" },
        { 43, "isatTable" },
        { 44, "isAlwaysOn" }
    };

}