<div align="center">

# Update Guide for KBF

These are some rough notes to help with handling game updates and ensuring all the bits of KBF work properly since there's currently no tests for it.

</div>

---

### Pull Latest REFramework

- REFramework tends to break with game updates, so this plugin likely will too until it's updated.

### Check CImGui Compatibility

- The version of CImGui used by REFramework may change with updates. If these mismatch, it can cause crashes in KBF's UI code.

### Check & Update Offsets for Manual Reads/Writes

- If the game crashes without any clear error in the logs, it's likely that some memory offsets have changed.
- Where we use manual memory read/writes, offsets often change between game versions.
- Do a search for hex values (i.e. `0x`) in the codebase to find these manual reads/writes.

### Fix logged errors in REFramework's log

- If an error is caused by an exception, it's stack trace will be logged in reframework's log.
- Run the stack trace addresses through `dia2dump.exe` via a script like `debug/resolve_symbols_example.bat` against the pdb for the relevant KBF version.
- This will tell you exactly where exceptions occur and makes it much easier to fix.

### Update the Max size of the NPC list

- The size of the NPC list may get larger with updates; not updating will mean certain npcs are never tracked.
- The size of the NPC list is defined in the game's Singleton `app.NpcManager > _NpcList`.
- Compile macro for it found in `npc/NpcTracker.hpp`

### Updating Armour Lists & NPC IDs.

- Update entries in `data/armour/armour_list.cpp`
- For NPCs, it might be necessary to add any new armour sets for them into their NPC ID Map: `npc/armour_id_to_npc.hpp` & `npc/get_npc_name_from_armour.hpp` (this really needs a better system)
- Make sure to ship a new `data/armour/armour_list.json` file with the new version of the mod.

### Common Code Problem Areas
- `XXX.applyPreset()` methods usually are the source of bugs after updates as they are the core functionality of the mod. Try disabling these first to identify the problem.

### File IO
- A bunch of stuff needs to be updated it `data/kbf_data_manager` to handle new NPC configs, settings, etc.
