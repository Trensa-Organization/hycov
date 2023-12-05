

This is a plugin fork originaly created by https://github.com/DreamMaoMao/hycov

## hyprshell
client overview for hyprland


### Manual Installation
tested with lastest hyprland version along with ags

##### use meson and ninja:

```console
$ git clone https://github.com/killown/hyprshell.git
$ cd hyprshell
$ sudo meson setup build --prefix=/usr
$ sudo ninja -C build
$ sudo ninja -C build install # `libhyprshell.so` path: /usr/lib/libhyprshell.so
```

##### use cmake:

```console
$ git clone https://github.com/killown/hyprshell.git
$ cd hyprshell
$ bash install.sh # `libhyprshell.so` path: /usr/lib/libhyprshell.so
```

### Usage(hyprland.conf)

```
# load the plugin in hyprland.conf
plugin = /usr/lib/libhyprshell.so

# when enter overview, you can use letf-button to jump,right-button to kill or use keybind
bind = CTRL_ALT,h,hyprshell:enteroverview
bind = CTRL_ALT,m,hyprshell:leaveoverview
bind = CTRL_ALT,k,hyprshell:toggleoverview

# The direction switch shortcut key binding.
# calculate the window closest to the direction to switch focus.
# This keybind is applicable not only to the overview  but also to the general layout
bind=ALT,left,hyprshell:movefocus,l
bind=ALT,right,hyprshell:movefocus,r
bind=ALT,up,hyprshell:movefocus,u
bind=ALT,down,hyprshell:movefocus,d


# plugin options
plugin {
    hyprshell {
        overview_gappo = 60 # gas width from screem 
        overview_gappi = 24 # gas width from clients
        hotarea_size = 100 # hotarea size in bottom left,10x10
        enable_hotarea = 1 # enable mouse cursor hotarea     
        swipe_fingers = 4 # finger number of gesture,move any directory
        move_focus_distance = 100 # distance for movefocus,only can use 3 finger to move 
        enable_gesture = 0 # enable gesture
        disable_workspace_change = 1 # disable workspace change when in overview mode
        enable_mouse_side_button = 1 # toggle overview with mouse side button
        enable_mouse_extra_button = 1 # kill active window
        disable_spawn = 0 # disable bind exec when in overview mode
        use_default_layout = 0 # if 1, will use default grid layout
        enable_keypress = 1 #enable keypress, num keys will move active window to the respective workspace number, esc leaves overview and so on...
        auto_exit = 1 # enable auto exit when no client in overview
    }
}

```


##### Features
- show only windows from active workspace
- toggle overview with side button
- kill focused window with extra button
- move focused window to another workspace by pressing the workspace number
- move focused window between workspaces with mouse middle click in the active window

