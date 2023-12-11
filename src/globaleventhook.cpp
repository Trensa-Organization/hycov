#include "GridLayout.hpp"
#include "dispatchers.hpp"
#include "globals.hpp"
#include <ctime>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <regex>
#include <set>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

unsigned int key_delay_ms = 100;
using namespace std;

typedef void (*origOnSwipeBegin)(void *, wlr_pointer_swipe_begin_event *e);
typedef void (*origOnSwipeEnd)(void *, wlr_pointer_swipe_end_event *e);
typedef void (*origOnSwipeUpdate)(void *, wlr_pointer_swipe_update_event *e);
typedef void (*origOnWindowRemovedTiling)(void *, CWindow *pWindow);

static double gesture_dx, gesture_previous_dx;
static double gesture_dy, gesture_previous_dy;

bool FullScreenMaximized = true;

static void hkOnSwipeUpdate(void *thisptr, wlr_pointer_swipe_update_event *e) {
  if (g_isOverView) {
    gesture_dx = gesture_dx + e->dx;
    gesture_dy = gesture_dy + e->dy;
    if (e->dx > 0 && gesture_dx - gesture_previous_dx > g_move_focus_distance) {
      dispatch_focusdir("r");
      gesture_previous_dx = gesture_dx;
      hyprshell_log(LOG, "OnSwipeUpdate hook focus right");
    } else if (e->dx < 0 &&
               gesture_previous_dx - gesture_dx > g_move_focus_distance) {
      dispatch_focusdir("l");
      gesture_previous_dx = gesture_dx;
      hyprshell_log(LOG, "OnSwipeUpdate hook focus left");
    } else if (e->dy > 0 &&
               gesture_dy - gesture_previous_dy > g_move_focus_distance) {
      dispatch_focusdir("d");
      gesture_previous_dy = gesture_dy;
      hyprshell_log(LOG, "OnSwipeUpdate hook focus down");
    } else if (e->dy < 0 &&
               gesture_previous_dy - gesture_dy > g_move_focus_distance) {
      dispatch_focusdir("u");
      gesture_previous_dy = gesture_dy;
      hyprshell_log(LOG, "OnSwipeUpdate hook focus up");
    }
    return;
  }
  (*(origOnSwipeUpdate)g_pOnSwipeUpdateHook->m_pOriginal)(thisptr, e);
}

static void hkOnSwipeBegin(void *thisptr, wlr_pointer_swipe_begin_event *e) {
  if (e->fingers == g_swipe_fingers) {
    g_isGestureBegin = true;
    return;
  }
  hyprshell_log(LOG, "OnSwipeBegin hook toggle");
  (*(origOnSwipeBegin)g_pOnSwipeBeginHook->m_pOriginal)(thisptr, e);
}

static void hkOnSwipeEnd(void *thisptr, wlr_pointer_swipe_end_event *e) {
  gesture_dx = 0;
  gesture_previous_dx = 0;
  gesture_dy = 0;
  gesture_previous_dy = 0;

  if (g_isGestureBegin) {
    g_isGestureBegin = false;
    dispatch_toggleoverview("");
    return;
  }
  hyprshell_log(LOG, "OnSwipeEnd hook toggle");
  (*(origOnSwipeEnd)g_pOnSwipeEndHook->m_pOriginal)(thisptr, e);
}

static void toggle_hotarea(int x_root, int y_root) {
  CMonitor *pMonitor = g_pCompositor->m_pLastMonitor;
  std::string arg = "";

  auto m_x = pMonitor->vecPosition.x;
  auto m_y = pMonitor->vecPosition.y;
  auto m_height = pMonitor->vecSize.y;

  int hx = m_x + g_hotarea_size;
  int hy = m_y + m_height - g_hotarea_size;

  if (!g_isInHotArea && y_root > hy && x_root < hx && x_root >= m_x &&
      y_root <= (m_y + m_height)) {
    hyprshell_log(LOG, "cursor enter hotarea");
    // dispatch_toggleoverview(arg);

    // hotcorner will switch window state for maximized or fullscreen

    if (!g_pCompositor->m_pLastWindow->m_bIsFullscreen ||
        FullScreenMaximized == true) {
      // g_pCompositor->setWindowFullscreen(g_pCompositor->m_pLastWindow, false,
      // FULLSCREEN_FULL);
      // g_pCompositor->setWindowFullscreen(g_pCompositor->m_pLastWindow, true,
      // FULLSCREEN_FULL);
      FullScreenMaximized = false;
      g_isInHotArea = true;
    } else {
      // g_pCompositor->setWindowFullscreen(g_pCompositor->m_pLastWindow, false,
      // FULLSCREEN_FULL);
      // g_pCompositor->setWindowFullscreen(g_pCompositor->m_pLastWindow, true,
      // FULLSCREEN_MAXIMIZED);
      FullScreenMaximized = true;
      g_isInHotArea = true;
    }
  } else if (g_isInHotArea && (y_root <= hy || x_root >= hx || x_root < m_x ||
                               y_root > (m_y + m_height))) {
    if (g_isInHotArea)
      g_isInHotArea = false;
  }
}

// reserve dock area mouse clicks
static void DockArea(int x_root, int y_root) {
  CMonitor *pMonitor = g_pCompositor->m_pLastMonitor;

  auto m_x = pMonitor->vecPosition.x;
  auto m_y = pMonitor->vecPosition.y;

  std::clog << (x_root) << std::endl;
  std::clog << (y_root) << std::endl;

  if (y_root > 1000) {
    g_isInDockArea = true;
  } else {
    g_isInDockArea = false;
  }
}

// reserve Top bar area for mouse clicks
static void TopBarArea(int x_root, int y_root) {
  CMonitor *pMonitor = g_pCompositor->m_pLastMonitor;

  auto m_x = pMonitor->vecPosition.x;
  auto m_y = pMonitor->vecPosition.y;

  std::clog << (x_root) << std::endl;
  std::clog << (y_root) << std::endl;

  if (y_root >= 0 && y_root < 80) {
    g_isInTopBarArea = true;
  } else {
    g_isInTopBarArea = false;
  }
}

int timestamp() {
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  int ms = 1000 * tp.tv_sec + tp.tv_nsec / 1000000;
  return ms;
}
static void parse_fixed(const char *d, wl_fixed_t *fixed) {
  char *end;
  int val = strtod(d, &end);
  if (end == d || *end) {
    return;
  } else {
    *fixed = wl_fixed_from_double(val);
  }
}

static void HotCornerActivation() {
  if (g_isHotCornerAvailable == true)
    // dispatch_toggleoverview("");
    // HyprlandAPI::invokeHyprctlCommand("dispatch", "exec wlrctl pointer move 4
    // 10"); wl_fixed_t *dx, *dy; struct zwlr_virtual_pointer_v1 *device;
    // parse_fixed("10", dx);
    // zwl_r_virtual_pointer_v1motion(device, timestamp(), dx, dy)
    g_isHotCornerAvailable = false;
}

static void HotCornerToggleOverviewArea(int x_root, int y_root) {
  CMonitor *pMonitor = g_pCompositor->m_pLastMonitor;

  auto m_x = pMonitor->vecPosition.x;
  auto m_y = pMonitor->vecPosition.y;

  if (y_root < 20 && x_root > g_hotarea_size && x_root < g_hotarea_size + 10) {
    // HotCornerActivation();
    // HyprlandAPI::invokeHyprctlCommand("dispatch", "exec wlrctl pointer move
    // 4");
    //  std::clog << (y_root) << std::endl;
    //  std::clog << (x_root) << std::endl;
    //  std::clog << (pMonitor->output->height) << std::endl;
    //  std::clog << (pMonitor->output->width) << std::endl;
  }
  // leave the hotcorner are will make the toggle overview available again
  if (y_root > 10 && y_root < 20) {
    g_isHotCornerAvailable = true;
  }
}

void moveActiveToWorkspace(std::string args) {

  CWindow *PWINDOW = nullptr;

  if (args.contains(',')) {
    PWINDOW = g_pCompositor->getWindowByRegex(
        args.substr(args.find_last_of(',') + 1));
    args = args.substr(0, args.find_last_of(','));
  } else {
    PWINDOW = g_pCompositor->m_pLastWindow;
  }

  if (!PWINDOW)
    return;

  // hack
  std::string workspaceName;
  const auto WORKSPACEID = getWorkspaceIDFromString(args, workspaceName);

  if (WORKSPACEID == WORKSPACE_INVALID) {
    Debug::log(LOG, "Invalid workspace in moveActiveToWorkspace");
    return;
  }

  if (WORKSPACEID == PWINDOW->m_iWorkspaceID) {
    Debug::log(LOG, "Not moving to workspace because it didn't change.");
    return;
  }

  auto pWorkspace = g_pCompositor->getWorkspaceByID(WORKSPACEID);
  CMonitor *pMonitor = nullptr;
  const auto POLDWS = g_pCompositor->getWorkspaceByID(PWINDOW->m_iWorkspaceID);
  static auto *const PALLOWWORKSPACECYCLES =
      &g_pConfigManager->getConfigValuePtr("binds:allow_workspace_cycles")
           ->intValue;

  g_pHyprRenderer->damageWindow(PWINDOW);

  if (pWorkspace) {
    g_pCompositor->moveWindowToWorkspaceSafe(PWINDOW, pWorkspace);
    pMonitor = g_pCompositor->getMonitorFromID(pWorkspace->m_iMonitorID);
    g_pCompositor->setActiveMonitor(pMonitor);
  } else {
    pWorkspace = g_pCompositor->createNewWorkspace(
        WORKSPACEID, PWINDOW->m_iMonitorID, workspaceName);
    pMonitor = g_pCompositor->getMonitorFromID(pWorkspace->m_iMonitorID);
    g_pCompositor->moveWindowToWorkspaceSafe(PWINDOW, pWorkspace);
  }

  POLDWS->m_pLastFocusedWindow =
      g_pCompositor->getFirstWindowOnWorkspace(POLDWS->m_iID);

  if (pWorkspace->m_bIsSpecialWorkspace)
    pMonitor->setSpecialWorkspace(pWorkspace);
  else if (POLDWS->m_bIsSpecialWorkspace)
    g_pCompositor->getMonitorFromID(POLDWS->m_iMonitorID)
        ->setSpecialWorkspace(nullptr);

  pMonitor->changeWorkspace(pWorkspace);

  g_pCompositor->focusWindow(PWINDOW);
  g_pCompositor->warpCursorTo(PWINDOW->middle());

  if (*PALLOWWORKSPACECYCLES)
    pWorkspace->rememberPrevWorkspace(POLDWS);
}

static void mouseMoveHook(void *, SCallbackInfo &info, std::any data) {

  const Vector2D coordinate = std::any_cast<const Vector2D>(data);
  HotCornerToggleOverviewArea(coordinate.x, coordinate.y);
  DockArea(coordinate.x, coordinate.y);
  TopBarArea(coordinate.x, coordinate.y);
}

static void mouseButtonHook(void *, SCallbackInfo &info, std::any data) {
  wlr_pointer_button_event *pEvent =
      std::any_cast<wlr_pointer_button_event *>(data);
  info.cancelled = false;

  switch (pEvent->button) {
  case BTN_LEFT:
    if (g_isOverView && pEvent->state == WLR_BUTTON_PRESSED &&
        g_isInDockArea == false) {
      dispatch_toggleoverview("");
      info.cancelled = true;
    }
    break;

  case BTN_RIGHT:
    if (g_isOverView && pEvent->state == WLR_BUTTON_PRESSED &&
        g_isInDockArea == false) {
      // dispatch_toggleoverview("");
      // HyprlandAPI::invokeHyprctlCommand("dispatch",
      //                                 "exec nwg-drawer -wm hyprland -c 8
      //                                 -k");
      // info.cancelled = true;
    }
    break;

  case BTN_SIDE:
    if (!g_isOverView && pEvent->state == WLR_BUTTON_PRESSED) {
      info.cancelled = true;
    }
    if (pEvent->state == WLR_BUTTON_PRESSED && g_enable_mouse_side_button) {

      dispatch_toggleoverview("");
      // HyprlandAPI::invokeHyprctlCommand("dispatch", "exec
      // /home/neo/.config/rofi/scripts/launcher_t6");
      info.cancelled = true;
    } else {
    }

    break;

  case BTN_EXTRA:
    if (g_isOverView && pEvent->state == WLR_BUTTON_PRESSED &&
        g_enable_mouse_extra_button) {
      HyprlandAPI::invokeHyprctlCommand("dispatch", "killactive");
      info.cancelled = true;
    }
    break;

  case BTN_MIDDLE:
    if (g_isOverView && pEvent->state == WLR_BUTTON_PRESSED) {
      const auto PMONITORTOCHANGETOLEFT =
          g_pCompositor->getMonitorInDirection('l');
      const auto PMONITORTOCHANGETORIGHT =
          g_pCompositor->getMonitorInDirection('r');

      if (!PMONITORTOCHANGETOLEFT && !PMONITORTOCHANGETORIGHT)
        return;

      if (!PMONITORTOCHANGETOLEFT) {
        const auto PWORKSPACERIGHT = g_pCompositor->getWorkspaceByID(
            PMONITORTOCHANGETORIGHT->activeWorkspace);
        moveActiveToWorkspace(PWORKSPACERIGHT->getConfigName());
        info.cancelled = true;
        return;
      }

      const auto PWORKSPACELEFT = g_pCompositor->getWorkspaceByID(
          PMONITORTOCHANGETOLEFT->activeWorkspace);
      moveActiveToWorkspace(PWORKSPACELEFT->getConfigName());
      info.cancelled = true;
    }
    break;
  }
}

static void moveWorkspaceWithHyprctl(std::string num) {
  dispatch_leaveoverview("");
  const std::string movetoworkspace = std::format("movetoworkspace {}", num);
  HyprlandAPI::invokeHyprctlCommand("dispatch", movetoworkspace);
}

static const char *get_keysym_name(xcb_keysym_t keysym) {
  static char name[64];
  xkb_keysym_get_name(keysym, name, sizeof(name));
  return name;
}

static bool isModPressed(const char *KEYNAME, wlr_keyboard_key_event *const e) {
  // does not continue if still pressed
  if (KEYNAME == keysym_Super_L || KEYNAME == keysym_Control_L ||
      KEYNAME == keysym_Control_R || KEYNAME == keysym_Shift_L ||
      KEYNAME == keysym_Shift_R) {
    if (e->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
      ModKeyStatus = true;
      return true;
    }
  }

  // continue if mod keys is released
  if (KEYNAME == keysym_Super_L || KEYNAME == keysym_Control_L ||
      KEYNAME == keysym_Control_R || KEYNAME == keysym_Shift_L ||
      KEYNAME == keysym_Shift_R) {
    if (e->state == WL_KEYBOARD_KEY_STATE_RELEASED) {
      ModKeyStatus = false;
      return false;
    }
    if (ModKeyStatus == true) {
      return true;
    } else {
      ModKeyStatus == false;
      return false;
    }
  }
}

// keyboard implementation, handle keypress events
static void keyPressHook(void *key_event, SCallbackInfo &info, std::any data) {
  info.cancelled = false;
  const auto DATA =
      std::any_cast<std::unordered_map<std::string, std::any>>(data);
  const auto PKEYBOARD = std::any_cast<SKeyboard *>(DATA.at("keyboard"));
  const auto state =
      wlr_keyboard_from_input_device(PKEYBOARD->keyboard)->xkb_state;
  const auto e = std::any_cast<wlr_keyboard_key_event *>(DATA.at("event"));
  const auto KEYCODE = e->keycode + 8;
  const auto KEYSYM = xkb_state_key_get_one_sym(state, KEYCODE);
  const auto KEYNAME = get_keysym_name(KEYSYM);
  const bool ModKeystate =
      isModPressed(KEYNAME, e); // check is the mod keys is pressed yet

  if (!PKEYBOARD->enabled) {
    return;
  }

  if (g_isOverView && g_enable_keypress) {
    // std::clog << (KEYNAME) << std::endl;
    // const auto notify = std::format("exec notify-send {}", KEYNAME);
    // HyprlandAPI::invokeHyprctlCommand("dispatch", notify);

    // leave overview with ESC, keycode 1 == ESC
    if (KEYNAME == keysym_ESC) {
      dispatch_toggleoverview("");
      info.cancelled = true;
    }

    // move to workspace using the given number
    if (KEYNAME == keysym_1) {
      moveWorkspaceWithHyprctl("1");
      info.cancelled = true;
    }

    if (KEYNAME == keysym_2) {
      moveWorkspaceWithHyprctl("2");
      info.cancelled = true;
    }
    if (KEYNAME == keysym_3) {
      moveWorkspaceWithHyprctl("3");
      info.cancelled = true;
    }

    if (KEYNAME == keysym_4) {
      moveWorkspaceWithHyprctl("4");
      info.cancelled = true;
    }
    if (KEYNAME == keysym_5) {
      moveWorkspaceWithHyprctl("5");
      info.cancelled = true;
    }

    if (KEYNAME == keysym_6) {
      moveWorkspaceWithHyprctl("6");
      info.cancelled = true;
    }
    if (KEYNAME == keysym_7) {
      moveWorkspaceWithHyprctl("7");
      info.cancelled = true;
    }

    if (KEYNAME == keysym_8) {
      moveWorkspaceWithHyprctl("8");
      info.cancelled = true;
    }
    if (KEYNAME == keysym_9) {
      moveWorkspaceWithHyprctl("9");
      info.cancelled = true;
    }

    list<string> menu_keys{keysym_A, keysym_B, keysym_C, keysym_D, keysym_E,
                           keysym_F, keysym_G, keysym_H, keysym_I, keysym_J,
                           keysym_K, keysym_L, keysym_M, keysym_N, keysym_O,
                           keysym_P, keysym_Q, keysym_R, keysym_S, keysym_T,
                           keysym_U, keysym_V, keysym_X, keysym_Y, keysym_Z};

    bool start_menu = (std::find(menu_keys.begin(), menu_keys.end(), KEYNAME) !=
                       menu_keys.end());
    if (start_menu && ModKeystate == false) {
      const auto drawer = std::format(
          "exec nwg-drawer -wm hyprland -c 8 -k -search {}", KEYNAME);
      HyprlandAPI::invokeHyprctlCommand("dispatch", drawer);
      dispatch_toggleoverview("");
    }
  }
}

static void mouseAxisHook(void *self, SCallbackInfo &info, std::any data) {

  if (g_isOverView) {
    info.cancelled = false;
    const auto DATA =
        std::any_cast<std::unordered_map<std::string, std::any>>(data);
    const auto e = std::any_cast<wlr_pointer_axis_event *>(DATA.at("event"));
    const auto PMONITORTOCHANGETORIGHT =
        g_pCompositor->getMonitorInDirection('r');
    const auto PWORKSPACERIGHT = g_pCompositor->getWorkspaceByID(
        PMONITORTOCHANGETORIGHT->activeWorkspace);
    const auto PMONITORTOCHANGETOLEFT =
        g_pCompositor->getMonitorInDirection('l');
    const auto PWORKSPACELEFT = g_pCompositor->getWorkspaceByID(
        PMONITORTOCHANGETOLEFT->activeWorkspace);

    if (!PMONITORTOCHANGETOLEFT && !PMONITORTOCHANGETORIGHT)
      return;

    // mouse wheel up, need implementation to move active window to workspace
    // right
    if (e->source == WLR_AXIS_SOURCE_WHEEL &&
        e->orientation == WLR_AXIS_ORIENTATION_VERTICAL) {
      if (e->delta < 0) {
        // moveActiveToWorkspace(PWORKSPACERIGHT->getConfigName());
        info.cancelled = true;
      }
    }

    // mouse wheel down, mouse wheel up, need implementation to move active
    // window to workspace left
    if (e->source == WLR_AXIS_SOURCE_WHEEL &&
        e->orientation == WLR_AXIS_ORIENTATION_HORIZONTAL) {
      if (e->delta > 0) {
        // moveActiveToWorkspace(PWORKSPACELEFT->getConfigName());
        info.cancelled = true;
      }
    }
  }
}

static void hkOnWindowRemovedTiling(void *thisptr, CWindow *pWindow) {
  (*(origOnWindowRemovedTiling)g_pOnWindowRemovedTilingHook->m_pOriginal)(
      thisptr, pWindow);

  if (g_isOverView && g_GridLayout->m_lGridNodesData.empty()) {
    hyprshell_log(LOG, "no tiling windwo,auto exit overview");
    dispatch_leaveoverview("");
  }
}

static void hkChangeworkspace(void *thisptr, std::string args) {
  hyprshell_log(LOG, "ChangeworkspaceHook hook toggle");
}

static void hkMoveActiveToWorkspace(void *thisptr, std::string args) {
  hyprshell_log(LOG, "MoveActiveToWorkspace hook toggle");
}

static void hkSpawn(void *thisptr, std::string args) {
  hyprshell_log(LOG, "Spawn hook toggle");
}

void registerGlobalEventHook() {
  // g_isInHotArea = false;
  g_isGestureBegin = false;
  g_isOverView = false;
  gesture_dx = 0;
  gesture_dy = 0;
  gesture_previous_dx = 0;
  gesture_previous_dy = 0;

  g_pOnSwipeBeginHook = HyprlandAPI::createFunctionHook(
      PHANDLE, (void *)&CInputManager::onSwipeBegin, (void *)&hkOnSwipeBegin);
  g_pOnSwipeEndHook = HyprlandAPI::createFunctionHook(
      PHANDLE, (void *)&CInputManager::onSwipeEnd, (void *)&hkOnSwipeEnd);
  g_pOnSwipeUpdateHook = HyprlandAPI::createFunctionHook(
      PHANDLE, (void *)&CInputManager::onSwipeUpdate, (void *)&hkOnSwipeUpdate);

  g_pOnWindowRemovedTilingHook = HyprlandAPI::createFunctionHook(
      PHANDLE, (void *)&GridLayout::onWindowRemovedTiling,
      (void *)&hkOnWindowRemovedTiling);

  // create private function hook
  static const auto ChangeworkspaceMethods =
      HyprlandAPI::findFunctionsByName(PHANDLE, "changeworkspace");
  g_pChangeworkspaceHook = HyprlandAPI::createFunctionHook(
      PHANDLE, ChangeworkspaceMethods[0].address, (void *)&hkChangeworkspace);

  static const auto MoveActiveToWorkspaceMethods =
      HyprlandAPI::findFunctionsByName(PHANDLE, "moveActiveToWorkspace");
  g_pMoveActiveToWorkspaceHook = HyprlandAPI::createFunctionHook(
      PHANDLE, MoveActiveToWorkspaceMethods[0].address,
      (void *)&hkMoveActiveToWorkspace);

  static const auto SpawnMethods =
      HyprlandAPI::findFunctionsByName(PHANDLE, "spawn");
  g_pSpawnHook = HyprlandAPI::createFunctionHook(
      PHANDLE, SpawnMethods[0].address, (void *)&hkSpawn);

  // register pEvent hook
  if (g_enable_hotarea) {
    HyprlandAPI::registerCallbackDynamic(
        PHANDLE, "mouseMove",
        [&](void *self, SCallbackInfo &info, std::any data) {
          mouseMoveHook(self, info, data);
        });
    HyprlandAPI::registerCallbackDynamic(
        PHANDLE, "mouseButton",
        [&](void *self, SCallbackInfo &info, std::any data) {
          mouseButtonHook(self, info, data);
        });
  }
  // HyprlandAPI::registerCallbackDynamic(PHANDLE, "mouseAxis", [&](void* self,
  // SCallbackInfo& info, std::any data) { mouseAxisHook(self, info, data); });
  HyprlandAPI::registerCallbackDynamic(
      PHANDLE, "keyPress",
      [&](void *key_event, SCallbackInfo &info, std::any data) {
        keyPressHook(key_event, info, data);
      });

  // enable function hook
  if (g_enable_gesture) {
    g_pOnSwipeBeginHook->hook();
    g_pOnSwipeEndHook->hook();
    g_pOnSwipeUpdateHook->hook();
  }

  // enable auto exit
  if (g_auto_exit) {
    g_pOnWindowRemovedTilingHook->hook();
  }
}