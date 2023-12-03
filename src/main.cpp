#include <optional>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include "dispatchers.hpp"
#include "globals.hpp"
#include "globaleventhook.hpp"

APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle)
{
	PHANDLE = handle;

   const std::string HASH = __hyprland_api_get_hash();

    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprwinwrap] Failure in initialization: Version mismatch (headers ver is not equal to running hyprland ver)",
                                     CColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hww] Version mismatch");
    }


#define CONF(NAME, TYPE, VALUE) \
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hypershell:" NAME, SConfigValue{.TYPE##Value = VALUE})

	CONF("overview_gappo", int, 60);
	CONF("overview_gappi", int, 24);
	CONF("hotarea_size", int, 10);
	CONF("enable_hotarea", int, 1);
	CONF("enable_mouse_side_button", int, 1);
	CONF("enable_mouse_extra_button", int, 1);
	CONF("use_default_layout", int, 1);
	CONF("swipe_fingers", int, 4);
	CONF("move_focus_distance", int, 100);
	CONF("enable_gesture", int, 1);
	CONF("disable_workspace_change", int, 1);
	CONF("disable_spawn", int, 1);
	CONF("auto_exit", int, 1);


#undef CONF

	static const auto *pEnable_hotarea_config = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hypershell:enable_hotarea")->intValue;
	static const auto *pEnable_enable_mouse_side_button_config = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hypershell:enable_mouse_side_button")->intValue;
	static const auto *pEnable_enable_mouse_extra_button_config = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hypershell:enable_mouse_extra_button")->intValue;
	static const auto *pEnable_g_use_default_layout = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hypershell:use_default_layout")->intValue;
  	static const auto *pHotarea_size_config = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hypershell:hotarea_size")->intValue;
	static const auto *pSwipe_fingers_config = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hypershell:swipe_fingers")->intValue;
	static const auto *pMove_focus_distance_config = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hypershell:move_focus_distance")->intValue;
	static const auto *pEnable_gesture_config = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hypershell:enable_gesture")->intValue;
	static const auto *pDisable_workspace_change_config = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hypershell:disable_workspace_change")->intValue;
	static const auto *pDisable_spawn_config = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hypershell:disable_spawn")->intValue;
	static const auto *pAuto_exit_config = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hypershell:auto_exit")->intValue;


	g_enable_hotarea = *pEnable_hotarea_config;
	g_enable_mouse_side_button = *pEnable_enable_mouse_side_button_config;
	g_enable_mouse_extra_button = *pEnable_enable_mouse_extra_button_config;
	g_use_default_layout = *pEnable_g_use_default_layout;
	g_hotarea_size = *pHotarea_size_config;
	g_swipe_fingers = *pSwipe_fingers_config;
	g_move_focus_distance = *pMove_focus_distance_config;
	g_enable_gesture = *pEnable_gesture_config;
	g_disable_workspace_change = *pDisable_workspace_change_config;
	g_disable_spawn = *pDisable_spawn_config;
	g_auto_exit = *pAuto_exit_config;


	g_GridLayout = std::make_unique<GridLayout>();
	HyprlandAPI::addLayout(PHANDLE, "grid", g_GridLayout.get());

	registerGlobalEventHook();
	registerDispatchers();

	return {"hypershell", "client overview", "killown", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
