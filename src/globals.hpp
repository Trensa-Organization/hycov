#pragma once

#include "GridLayout.hpp"
#include "log.hpp"

#include <hyprland/src/plugins/PluginAPI.hpp>

inline HANDLE PHANDLE = nullptr;
inline std::unique_ptr<GridLayout> g_GridLayout;

inline bool g_isOverView;
inline bool g_isInHotArea;
inline int g_enable_hotarea;
inline int g_enable_mouse_side_button;
inline int g_enable_mouse_extra_button;
inline int g_use_default_layout;
inline int g_menu_app_exec;
inline int g_enable_keypress;
inline int g_hotarea_size;
inline int g_swipe_fingers;
inline int g_isGestureBegin;
inline int g_move_focus_distance;
inline int g_enable_gesture;
inline int g_disable_workspace_change;
inline int g_disable_spawn;
inline int g_auto_exit;
inline std::string keysym_ESC = "Escape";
inline std::string keysym_A = "a";
inline std::string keysym_B = "b";
inline std::string keysym_C = "c";
inline std::string keysym_D = "d";
inline std::string keysym_E = "e";
inline std::string keysym_F = "f";
inline std::string keysym_G = "g";
inline std::string keysym_H = "h";
inline std::string keysym_I = "i";
inline std::string keysym_J = "j";
inline std::string keysym_K = "k";
inline std::string keysym_L = "l";
inline std::string keysym_M = "m";
inline std::string keysym_N = "n";
inline std::string keysym_O = "o";
inline std::string keysym_P = "p";
inline std::string keysym_Q = "q";
inline std::string keysym_R = "r";
inline std::string keysym_S = "s";
inline std::string keysym_T = "t";
inline std::string keysym_U = "u";
inline std::string keysym_V = "v";
inline std::string keysym_X = "";
inline std::string keysym_Y = "y";
inline std::string keysym_Z = "z";
inline std::string keysym_1 = "1";
inline std::string keysym_2 = "2";
inline std::string keysym_3 = "3";
inline std::string keysym_4 = "4";
inline std::string keysym_5 = "5";
inline std::string keysym_6 = "6";
inline std::string keysym_7 = "7";
inline std::string keysym_8 = "8";
inline std::string keysym_9 = "9";


inline CFunctionHook* g_pOnSwipeBeginHook = nullptr;
inline CFunctionHook* g_pOnSwipeEndHook = nullptr;
inline CFunctionHook* g_pOnSwipeUpdateHook = nullptr;
inline CFunctionHook* g_pOnWindowRemovedTilingHook = nullptr;
inline CFunctionHook* g_pChangeworkspaceHook = nullptr;
inline CFunctionHook* g_pMoveActiveToWorkspaceHook = nullptr;
inline CFunctionHook* g_pSpawnHook = nullptr;

inline void errorNotif()
{
	HyprlandAPI::addNotificationV2(
		PHANDLE,
		{
			{"text", "Something has gone very wrong. Check the log for details."},
			{"time", (uint64_t)10000},
			{"color", CColor(1.0, 0.0, 0.0, 1.0)},
			{"icon", ICON_ERROR},
		});
}
