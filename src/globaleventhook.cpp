#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <regex>
#include <xkbcommon/xkbcommon.h>
#include <set>
#include "dispatchers.hpp"
#include "globals.hpp"
#include "GridLayout.hpp"


typedef void (*origOnSwipeBegin)(void*, wlr_pointer_swipe_begin_event* e);
typedef void (*origOnSwipeEnd)(void*, wlr_pointer_swipe_end_event* e);
typedef void (*origOnSwipeUpdate)(void*, wlr_pointer_swipe_update_event* e);
typedef void (*origOnWindowRemovedTiling)(void*, CWindow *pWindow);

static double gesture_dx,gesture_previous_dx;
static double gesture_dy,gesture_previous_dy;

bool FullScreenMaximized = true;



static void hkOnSwipeUpdate(void* thisptr, wlr_pointer_swipe_update_event* e) {
  if(g_isOverView){
    gesture_dx = gesture_dx + e->dx;
    gesture_dy = gesture_dy + e->dy;
    if(e->dx > 0 && gesture_dx - gesture_previous_dx > g_move_focus_distance){
      dispatch_focusdir("r");
      gesture_previous_dx = gesture_dx;
      hycov_log(LOG,"OnSwipeUpdate hook focus right");
    } else if(e->dx < 0 && gesture_previous_dx - gesture_dx > g_move_focus_distance){
      dispatch_focusdir("l");
      gesture_previous_dx = gesture_dx;
      hycov_log(LOG,"OnSwipeUpdate hook focus left");
    } else if(e->dy > 0 && gesture_dy - gesture_previous_dy > g_move_focus_distance){
      dispatch_focusdir("d");
      gesture_previous_dy = gesture_dy;
      hycov_log(LOG,"OnSwipeUpdate hook focus down");
    } else if(e->dy < 0 && gesture_previous_dy - gesture_dy > g_move_focus_distance){
      dispatch_focusdir("u");
      gesture_previous_dy = gesture_dy;
      hycov_log(LOG,"OnSwipeUpdate hook focus up");
    }
    return;
  }
  (*(origOnSwipeUpdate)g_pOnSwipeUpdateHook->m_pOriginal)(thisptr, e);
}

static void hkOnSwipeBegin(void* thisptr, wlr_pointer_swipe_begin_event* e) {
  if(e->fingers == g_swipe_fingers){
   g_isGestureBegin = true;
    return;
  } 
  hycov_log(LOG,"OnSwipeBegin hook toggle");
  (*(origOnSwipeBegin)g_pOnSwipeBeginHook->m_pOriginal)(thisptr, e);
}

static void hkOnSwipeEnd(void* thisptr, wlr_pointer_swipe_end_event* e) {
  gesture_dx = 0;
  gesture_previous_dx = 0;
  gesture_dy = 0;
  gesture_previous_dy = 0;
  
  if(g_isGestureBegin){
    g_isGestureBegin = false;
    dispatch_toggleoverview("");
    return;
  }
  hycov_log(LOG,"OnSwipeEnd hook toggle");
  (*(origOnSwipeEnd)g_pOnSwipeEndHook->m_pOriginal)(thisptr, e);
}

static void toggle_hotarea(int x_root, int y_root)
{
  CMonitor *pMonitor = g_pCompositor->m_pLastMonitor;
  std::string arg = "";

  auto m_x = pMonitor->vecPosition.x;
  auto m_y = pMonitor->vecPosition.y;
  auto m_height = pMonitor->vecSize.y;


  int hx = m_x + g_hotarea_size;
  int hy = m_y + m_height - g_hotarea_size;

  if (!g_isInHotArea && y_root > hy &&
      x_root < hx && x_root >= m_x &&
      y_root <= (m_y + m_height))
  {
    hycov_log(LOG,"cursor enter hotarea");
    //dispatch_toggleoverview(arg);

    //hotcorner will switch window state for maximized or fullscreen


    if(!g_pCompositor->m_pLastWindow->m_bIsFullscreen || FullScreenMaximized == true){
        g_pCompositor->setWindowFullscreen(g_pCompositor->m_pLastWindow, false, FULLSCREEN_FULL);
        g_pCompositor->setWindowFullscreen(g_pCompositor->m_pLastWindow, true, FULLSCREEN_FULL);
        FullScreenMaximized = false;
        g_isInHotArea = true;
    } else {
        g_pCompositor->setWindowFullscreen(g_pCompositor->m_pLastWindow, false, FULLSCREEN_FULL);
        g_pCompositor->setWindowFullscreen(g_pCompositor->m_pLastWindow, true, FULLSCREEN_MAXIMIZED);
        FullScreenMaximized = true;
        g_isInHotArea = true;
    }
    
  }
  else if (g_isInHotArea &&
           (y_root <= hy || x_root >= hx || x_root < m_x ||
            y_root > (m_y + m_height)))
  {
    if(g_isInHotArea)
      g_isInHotArea = false;
  }
}

void moveActiveToWorkspace(std::string args) {

    CWindow* PWINDOW = nullptr;

    if (args.contains(',')) {
        PWINDOW = g_pCompositor->getWindowByRegex(args.substr(args.find_last_of(',') + 1));
        args    = args.substr(0, args.find_last_of(','));
    } else {
        PWINDOW = g_pCompositor->m_pLastWindow;
    }

    if (!PWINDOW)
        return;

    // hack
    std::string workspaceName;
    const auto  WORKSPACEID = getWorkspaceIDFromString(args, workspaceName);

    if (WORKSPACEID == WORKSPACE_INVALID) {
        Debug::log(LOG, "Invalid workspace in moveActiveToWorkspace");
        return;
    }

    if (WORKSPACEID == PWINDOW->m_iWorkspaceID) {
        Debug::log(LOG, "Not moving to workspace because it didn't change.");
        return;
    }

    auto               pWorkspace            = g_pCompositor->getWorkspaceByID(WORKSPACEID);
    CMonitor*          pMonitor              = nullptr;
    const auto         POLDWS                = g_pCompositor->getWorkspaceByID(PWINDOW->m_iWorkspaceID);
    static auto* const PALLOWWORKSPACECYCLES = &g_pConfigManager->getConfigValuePtr("binds:allow_workspace_cycles")->intValue;

    g_pHyprRenderer->damageWindow(PWINDOW);

    if (pWorkspace) {
        g_pCompositor->moveWindowToWorkspaceSafe(PWINDOW, pWorkspace);
        pMonitor = g_pCompositor->getMonitorFromID(pWorkspace->m_iMonitorID);
        g_pCompositor->setActiveMonitor(pMonitor);
    } else {
        pWorkspace = g_pCompositor->createNewWorkspace(WORKSPACEID, PWINDOW->m_iMonitorID, workspaceName);
        pMonitor   = g_pCompositor->getMonitorFromID(pWorkspace->m_iMonitorID);
        g_pCompositor->moveWindowToWorkspaceSafe(PWINDOW, pWorkspace);
    }

    POLDWS->m_pLastFocusedWindow = g_pCompositor->getFirstWindowOnWorkspace(POLDWS->m_iID);

    if (pWorkspace->m_bIsSpecialWorkspace)
        pMonitor->setSpecialWorkspace(pWorkspace);
    else if (POLDWS->m_bIsSpecialWorkspace)
        g_pCompositor->getMonitorFromID(POLDWS->m_iMonitorID)->setSpecialWorkspace(nullptr);

    pMonitor->changeWorkspace(pWorkspace);

    g_pCompositor->focusWindow(PWINDOW);
    g_pCompositor->warpCursorTo(PWINDOW->middle());

    if (*PALLOWWORKSPACECYCLES)
        pWorkspace->rememberPrevWorkspace(POLDWS);
}


static void mouseMoveHook(void *, SCallbackInfo &info, std::any data)
{

  const Vector2D coordinate = std::any_cast<const Vector2D>(data);
  toggle_hotarea(coordinate.x, coordinate.y);
}

static void mouseButtonHook(void *, SCallbackInfo &info, std::any data)
{
  wlr_pointer_button_event *pEvent = std::any_cast<wlr_pointer_button_event *>(data); 
  info.cancelled = false;


  switch (pEvent->button)
  {
  case BTN_LEFT:
    if (g_isOverView && pEvent->state == WLR_BUTTON_PRESSED)
    {
      dispatch_toggleoverview("");
      info.cancelled = true;
    }
    break;



  case BTN_MIDDLE:
    if (g_isOverView && pEvent->state == WLR_BUTTON_PRESSED)
    {
       const auto PMONITORTOCHANGETOLEFT = g_pCompositor->getMonitorInDirection('l');
       const auto PMONITORTOCHANGETORIGHT = g_pCompositor->getMonitorInDirection('r');
      

        if (!PMONITORTOCHANGETOLEFT && !PMONITORTOCHANGETORIGHT)
          return;


       if (!PMONITORTOCHANGETOLEFT) {   
          const auto PWORKSPACERIGHT = g_pCompositor->getWorkspaceByID(PMONITORTOCHANGETORIGHT->activeWorkspace);    
          moveActiveToWorkspace(PWORKSPACERIGHT->getConfigName());
          info.cancelled = true;
          return;
       }

      const auto PWORKSPACELEFT = g_pCompositor->getWorkspaceByID(PMONITORTOCHANGETOLEFT->activeWorkspace); 
      moveActiveToWorkspace(PWORKSPACELEFT->getConfigName());
      info.cancelled = true;
    }
    break;

  }
}

//keyboard implementation, handle keypress events
static void keyPressHook(void* key_event, SCallbackInfo &info, std::any data)
{
 
     const auto DATA = std::any_cast<std::unordered_map<std::string, std::any>>(data);
     const auto PKEYBOARD = std::any_cast<SKeyboard*>(DATA.at("keyboard"));
     const auto state = wlr_keyboard_from_input_device(PKEYBOARD->keyboard)->xkb_state;
     const auto e = std::any_cast<wlr_keyboard_key_event*>(DATA.at("event"));

     if(!PKEYBOARD->enabled){
          return;
     }

     if (xkb_state_key_get_one_sym(state, e->keycode) == 0 && g_isOverView){ //keycode 0 == ESC
            dispatch_toggleoverview("");
            info.cancelled = true;
      }      
}



static void mouseAxisHook(void* self, SCallbackInfo &info, std::any data)
{
   const auto DATA = std::any_cast<std::unordered_map<std::string, std::any>>(data);
   const auto e = std::any_cast<wlr_pointer_axis_event*>(DATA.at("event"));

  //mouse wheel up, need implementation to move active window to workspace right
  if (e->source == WLR_AXIS_SOURCE_WHEEL && e->orientation == WLR_AXIS_ORIENTATION_VERTICAL && g_isOverView) {
      if (e->delta < 0){
          dispatch_toggleoverview("");
          info.cancelled = true;
     }
  }
  
  //mouse wheel down, mouse wheel up, need implementation to move active window to workspace left
  if (e->source == WLR_AXIS_SOURCE_WHEEL && e->orientation == WLR_AXIS_ORIENTATION_HORIZONTAL && g_isOverView) {
      if (e->delta < 0){
          dispatch_toggleoverview("");
          info.cancelled = true;
     }
  }
 
}


static void hkOnWindowRemovedTiling(void* thisptr, CWindow *pWindow) {
  (*(origOnWindowRemovedTiling)g_pOnWindowRemovedTilingHook->m_pOriginal)(thisptr, pWindow);

  if (g_isOverView && g_GridLayout->m_lGridNodesData.empty()) {
    hycov_log(LOG,"no tiling windwo,auto exit overview");
    dispatch_leaveoverview("");
  }
}

static void hkChangeworkspace(void* thisptr, std::string args) {
  hycov_log(LOG,"ChangeworkspaceHook hook toggle");
}

static void hkMoveActiveToWorkspace(void* thisptr, std::string args) {
  hycov_log(LOG,"MoveActiveToWorkspace hook toggle");
}

static void hkSpawn(void* thisptr, std::string args) {
  hycov_log(LOG,"Spawn hook toggle");
}

void registerGlobalEventHook()
{
  g_isInHotArea = false;
  g_isGestureBegin = false;
  g_isOverView = false;
  gesture_dx = 0;
  gesture_dy = 0;
  gesture_previous_dx = 0;
  gesture_previous_dy = 0;
  

  g_pOnSwipeBeginHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CInputManager::onSwipeBegin, (void*)&hkOnSwipeBegin);
  g_pOnSwipeEndHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CInputManager::onSwipeEnd, (void*)&hkOnSwipeEnd);
  g_pOnSwipeUpdateHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&CInputManager::onSwipeUpdate, (void*)&hkOnSwipeUpdate);

  g_pOnWindowRemovedTilingHook = HyprlandAPI::createFunctionHook(PHANDLE, (void*)&GridLayout::onWindowRemovedTiling, (void*)&hkOnWindowRemovedTiling);

  //create private function hook
  static const auto ChangeworkspaceMethods = HyprlandAPI::findFunctionsByName(PHANDLE, "changeworkspace");
  g_pChangeworkspaceHook = HyprlandAPI::createFunctionHook(PHANDLE, ChangeworkspaceMethods[0].address, (void*)&hkChangeworkspace);

  static const auto MoveActiveToWorkspaceMethods = HyprlandAPI::findFunctionsByName(PHANDLE, "moveActiveToWorkspace");
  g_pMoveActiveToWorkspaceHook = HyprlandAPI::createFunctionHook(PHANDLE, MoveActiveToWorkspaceMethods[0].address, (void*)&hkMoveActiveToWorkspace);

  static const auto SpawnMethods = HyprlandAPI::findFunctionsByName(PHANDLE, "spawn");
  g_pSpawnHook = HyprlandAPI::createFunctionHook(PHANDLE, SpawnMethods[0].address, (void*)&hkSpawn);

  //register pEvent hook
  if(g_enable_hotarea){
    HyprlandAPI::registerCallbackDynamic(PHANDLE, "mouseMove",[&](void* self, SCallbackInfo& info, std::any data) { mouseMoveHook(self, info, data); });
    HyprlandAPI::registerCallbackDynamic(PHANDLE, "mouseButton", [&](void* self, SCallbackInfo& info, std::any data) { mouseButtonHook(self, info, data); });
  }
   HyprlandAPI::registerCallbackDynamic(PHANDLE, "mouseAxis", [&](void* self, SCallbackInfo& info, std::any data) { mouseAxisHook(self, info, data); });
  HyprlandAPI::registerCallbackDynamic(PHANDLE, "keyPress", [&](void* key_event, SCallbackInfo& info, std::any data) { keyPressHook(key_event, info, data); });
 
  //enable function hook
  if(g_enable_gesture){
    g_pOnSwipeBeginHook->hook();
    g_pOnSwipeEndHook->hook();
    g_pOnSwipeUpdateHook->hook();
  }

  //enable auto exit
  if(g_auto_exit){
    g_pOnWindowRemovedTilingHook->hook();
  }

}
