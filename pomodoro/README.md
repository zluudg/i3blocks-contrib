# pomodoro
I simple 25/5 pomodoro timer. Left click on the orange circle to start a
25 minute timer and on the purple to start a 5 minute timer. Right clicking
on the block cancels the current timer. Alerts by raising an i3-nagbar warning.
Click block to reset to initial state after alert has been raised.

![](screenshot.png)

##  Build
```
make
```

## Config
```
[pomodoro]
command=$SCRIPT_DIR/pomodoro/pomodoro
interval=persist
format=json
markup=pango
```
