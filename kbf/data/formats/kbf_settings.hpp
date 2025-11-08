#pragma once

namespace kbf {

	struct KBFSettings {
		bool  enabled                        = true;
		bool  enablePlayers                  = true;
		bool  enableNpcs                     = true;
		float delayOnEquip                   = 0.050;
		float applicationRange               = 30.0f;
		int   maxConcurrentApplications      = 10;
		int   maxBoneFetchesPerFrame         = 1;
		bool  enableDuringQuestsOnly         = false;
		bool  enableHideWeapons              = true;
		bool  forceShowWeaponInTent          = true;
		bool  forceShowWeaponWhenSharpening  = true;
		bool  forceShowWeaponWhenOnSeikret   = true;
		bool  hideWeaponsOutsideOfCombatOnly = true;
		bool  hideSlingerOutsideOfCombatOnly = true;
		bool  enableProfiling                = false;
	};

}