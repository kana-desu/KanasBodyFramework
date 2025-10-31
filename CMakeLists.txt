cmake_minimum_required(VERSION 3.15)

set(VER 1.0.2)

project(
    KanaBodyFramework
    LANGUAGES C CXX
    VERSION ${VER}
)

message(STATUS "Source Dir: " ${PROJECT_SOURCE_DIR})
message(STATUS "Version: " ${VER})

# --- Packaging Options --------------------------------------------------------------------------

option(PACKAGE_FOR_DIST "Package build for distribution (v${VER})" OFF)

# --- Compiler Definitions -----------------------------------------------------------------------

add_compile_definitions(KBF_VERSION=\"${VER}\")
add_compile_definitions(NOMINMAX)

# --- External Lib Paths --------------------------------------------------------------------------

set(REFRAMEWORK_PATH   external/reframework)     
set(RAPIDJSON_PATH     external/rapidjson)         # RAPIDJSON
set(GLM_PATH           external/glm)               # GLM

# -------------------------------------------------------------------------------------------------

set(KBF_SOURCES
    "kbf/data/armour/armour_list.cpp"
    "kbf/data/bones/bone_cache_manager.cpp"
    "kbf/data/mesh/part_cache_manager.cpp"
    "kbf/data/kbf_data_manager.cpp"
    "kbf/gui/components/toggle/imgui_toggle.cpp"
    "kbf/gui/components/toggle/imgui_toggle_palette.cpp"
    "kbf/gui/components/toggle/imgui_toggle_presets.cpp"
    "kbf/gui/components/toggle/imgui_toggle_renderer.cpp"
    "kbf/gui/panels/info/info_popup_panel.cpp"
    "kbf/gui/panels/lists/bone_panel.cpp"
    "kbf/gui/panels/lists/part_remover_panel.cpp"
    "kbf/gui/panels/lists/player_list_panel.cpp"
    "kbf/gui/panels/lists/preset_group_panel.cpp"
    "kbf/gui/panels/lists/preset_panel.cpp"
    "kbf/gui/panels/presets/create_preset_group_from_bundle_panel.cpp"
    "kbf/gui/panels/presets/create_preset_group_panel.cpp"
    "kbf/gui/panels/presets/create_preset_panel.cpp"
    "kbf/gui/panels/presets/edit_player_override_panel.cpp"
    "kbf/gui/panels/presets/edit_preset_group_panel.cpp"
    "kbf/gui/panels/presets/edit_preset_panel.cpp"
    "kbf/gui/panels/presets/import_fbs_presets_panel.cpp"
    "kbf/gui/panels/share/export_panel.cpp"
    "kbf/gui/tabs/debug/debug_tab.cpp"
    "kbf/gui/tabs/editor/editor_tab.cpp"
    "kbf/gui/tabs/npc/npc_tab.cpp"
    "kbf/gui/tabs/player/player_tab.cpp"
    "kbf/gui/tabs/presets/presets_tab.cpp"
    "kbf/gui/tabs/preset_groups/preset_groups_tab.cpp"
    "kbf/gui/tabs/settings/settings_tab.cpp"
    "kbf/gui/tabs/share/share_tab.cpp"
    "kbf/gui/tabs/about/about_tab.cpp"
    "kbf/gui/kbf_window.cpp"
    "kbf/mesh/bone_manager.cpp"
    "kbf/mesh/part_manager.cpp"
    "kbf/npc/npc_tracker.cpp" 
    "kbf/player/player_tracker.cpp" 
    "kbf/profiling/cpu_profiler.cpp"
    "kbf/situation/situation_watcher.cpp"
    "kbf/watchers/fs_watcher_win.cpp"
    "kbf/watchers/kbf_dll_update_listener.cpp"
    "kbf/kbf.cpp" 
)

set(KBF_HOT_RELOADER_SOURCES
    "kbf/watchers/fs_watcher_win.cpp"
    "kbf/watchers/kbf_dll_update_listener.cpp"
)

# ------------------------------------------------------------------------------
# Choose build mode
# ------------------------------------------------------------------------------

if((CMAKE_BUILD_TYPE STREQUAL "Debug") OR (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"))
    message(STATUS "⚙️ Debug build: hot reload loader + logic DLL")
    message(STATUS "Hot Reload Dir: " ${PLUGIN_HOT_RELOAD_DIR})

    add_compile_definitions(KBF_DEBUG_BUILD)

    # 1. Build the logic DLL (internal target)
    add_library(${PROJECT_NAME}_logic SHARED ${KBF_SOURCES} main.cpp)
    target_compile_features(${PROJECT_NAME}_logic PUBLIC cxx_std_20)
    target_include_directories(${PROJECT_NAME}_logic 
        PUBLIC
            ${REFRAMEWORK_PATH}/include
            ${PROJECT_SOURCE_DIR}/${RAPIDJSON_PATH}/include
            ${PROJECT_SOURCE_DIR}/${GLM_PATH}
        PRIVATE
            ${PROJECT_SOURCE_DIR}/
    )

    # Note: Copying the PDB file first IS IMPORTANT so the hot-reloader picks it up when it reads a dll change.
    if(DEFINED PLUGIN_HOT_RELOAD_DIR)
        add_custom_command(TARGET ${PROJECT_NAME}_logic POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGIN_HOT_RELOAD_DIR}"
            COMMAND ${CMAKE_COMMAND} -E rm -f
                "${PLUGIN_HOT_RELOAD_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}_logic>.pdb"
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_PDB_FILE:${PROJECT_NAME}_logic>"
                "${PLUGIN_HOT_RELOAD_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}_logic>.pdb"
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:${PROJECT_NAME}_logic>"
                "${PLUGIN_HOT_RELOAD_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}_logic>"
        )

    endif()

    # 2. The *real* target the user builds: the loader DLL
    add_library(${PROJECT_NAME} SHARED ${KBF_HOT_RELOADER_SOURCES} main_hot_reload.cpp)
    target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
    target_include_directories(${PROJECT_NAME} 
        PUBLIC
            ${REFRAMEWORK_PATH}/include
            ${PROJECT_SOURCE_DIR}/${RAPIDJSON_PATH}/include
            ${PROJECT_SOURCE_DIR}/${GLM_PATH}
        PRIVATE
            ${PROJECT_SOURCE_DIR}/
    )
    # Pass DLL name for loader to use
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE LOGIC_DLL_NAME="$<TARGET_FILE_NAME:${PROJECT_NAME}_logic>")

    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_logic)

    if(DEFINED PLUGIN_OUTPUT_DIR AND IS_DIRECTORY "${PLUGIN_OUTPUT_DIR}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:${PROJECT_NAME}>"
                "${PLUGIN_OUTPUT_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
            COMMAND ${CMAKE_COMMAND} -E rm -f
                "${PLUGIN_OUTPUT_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>.pdb"
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:${PROJECT_NAME}>"
                "${PLUGIN_OUTPUT_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>.pdb"
        )
    endif()

    get_target_property(_incs ${PROJECT_NAME} INCLUDE_DIRECTORIES)
    message(STATUS "Include dirs for ${PROJECT_NAME}: ${_incs}")

else()
    message(STATUS "🚀 Release build: single plugin (no hot-loader)")

    add_library(${PROJECT_NAME} SHARED ${KBF_SOURCES} main.cpp)
    target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
    target_include_directories(${PROJECT_NAME} 
        PUBLIC
            ${REFRAMEWORK_PATH}/include
            ${PROJECT_SOURCE_DIR}/${RAPIDJSON_PATH}/include
            ${PROJECT_SOURCE_DIR}/${GLM_PATH}
        PRIVATE
            ${PROJECT_SOURCE_DIR}/
    )

    if(NOT PACKAGE_FOR_DIST AND DEFINED PLUGIN_OUTPUT_DIR AND IS_DIRECTORY "${PLUGIN_OUTPUT_DIR}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:${PROJECT_NAME}>"
                "${PLUGIN_OUTPUT_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
            COMMAND ${CMAKE_COMMAND} -E rm -f
                "${PLUGIN_OUTPUT_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>.pdb"
        )
    endif()

    get_target_property(_incs ${PROJECT_NAME} INCLUDE_DIRECTORIES)
    message(STATUS "Include dirs for ${PROJECT_NAME}: ${_incs}")

endif()


# ------------------------------------------------------------------------------
# Post Build Packaging
# ------------------------------------------------------------------------------

# Detect debug / release for packaging check
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:VALID_PACKAGING_BUILD_TYPE=False>
    $<$<CONFIG:RelWithDebInfo>:VALID_PACKAGING_BUILD_TYPE=False>
    $<$<CONFIG:Release>:VALID_PACKAGING_BUILD_TYPE=True>
    $<$<CONFIG:MinSizeRel>:VALID_PACKAGING_BUILD_TYPE=True>
)

# Package for Distribution
if(PACKAGE_FOR_DIST)
    if (NOT ${VALID_PACKAGING_BUILD_TYPE}) 
        message(FATAL_ERROR "Cannot create distribution package from a debug build.")
    endif()
    
    set(DIST_DIR ${CMAKE_SOURCE_DIR}/dist/out)
    set(PACKAGE_NAME ${DIST_DIR}/KanasBodyFramework_v${VER}.zip)

    # 1. Copy .dll to dist/out
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CMAKE_SOURCE_DIR}/dist/KBF/reframework/plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    )

    # 2. Zip up the folder.
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND 
            ${CMAKE_COMMAND} -E make_directory ${DIST_DIR} && 
            ${CMAKE_COMMAND} -E tar "cf" ${PACKAGE_NAME} --format=zip .
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dist/KBF
        COMMENT "Packaging Release Ver ${Ver}"
        VERBATIM)

    message(STATUS "PACKAGING READY: Build will be packaged to " ${PACKAGE_NAME})

endif()