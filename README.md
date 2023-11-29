## hypershell

clients overview for hyprland plugin


### Manual Installation
tested with lastest hyprland version along with ags

##### use meson and ninja:

```console
$ git clone https://github.com/DreamMaoMao/hycov.git
$ cd hycov
$ sudo meson setup build --prefix=/usr
$ sudo ninja -C build
$ sudo ninja -C build install # `libhycov.so` path: /usr/lib/libhycov.so
```

##### use cmake:

```console
$ git clone https://github.com/DreamMaoMao/hycov.git
$ cd hycov
$ bash install.sh # `libhycov.so` path: /usr/lib/libhycov.so
```

### Usage(hyprland.conf)

```
# when enter overview, you can use letf-button to jump,right-button to kill or use keybind
plugin = /path/to/libhycov.so
bind = CTRL_ALT,h,hycov:enteroverview
bind = CTRL_ALT,m,hycov:leaveoverview
bind = CTRL_ALT,k,hycov:toggleoverview

# The direction switch shortcut key binding.
# calculate the window closest to the direction to switch focus.
# This keybind is applicable not only to the overview  but also to the general layout
bind=ALT,left,hycov:movefocus,l
bind=ALT,right,hycov:movefocus,r
bind=ALT,up,hycov:movefocus,u
bind=ALT,down,hycov:movefocus,d

#use this for a while, this plugin have trouble using plugin = /usr/lib/libhycov.so which will crash hyprland
exec-once=sleep 4;hyprctl plugin load /usr/lib/libhycov.so


plugin {
    hycov {
        overview_gappo = 60 # gas width from screem 
        overview_gappi = 24 # gas width from clients
	      hotarea_size = 10 # hotarea size in bottom left,10x10
	      enable_hotarea = 1 # enable mouse cursor hotarea     
        swipe_fingers = 4 # finger number of gesture,move any directory
        move_focus_distance = 100 # distance for movefocus,only can use 3 finger to move 
        enable_gesture = 0 # enable gesture
        disable_workspace_change = 0 # disable workspace change when in overview mode
        disable_spawn = 0 # disable bind exec when in overview mode
        auto_exit = 1 # enable auto exit when no client in overview
    }
}

```


this great plugin was made by https://github.com/DreamMaoMao/hycov, thank you for the great work
