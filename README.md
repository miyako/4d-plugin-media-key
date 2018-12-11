# 4d-plugin-media-key
Capture media key press events

### Platform

| carbon | cocoa | win32 | win64 |
|:------:|:-----:|:---------:|:---------:|
||<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|

__Mac version is 64-bit only!__ 

### Version

<img src="https://cloud.githubusercontent.com/assets/1725068/18940648/2192ddba-8645-11e6-864d-6d5692d55717.png" width="32" height="32" /> <img src="https://user-images.githubusercontent.com/1725068/41266195-ddf767b2-6e30-11e8-9d6b-2adf6a9f57a5.png" width="32" height="32" />

## Syntax

```
ON MEDIA KEY CALL (method;context{;scope})
```

Parameter|Type|Description
------------|------------|----
method|TEXT|project method
context|TEXT|any text
scope|LONGINT|``KEYEVENT_LOCAL`` or ``KEYEVENT_GLOBAL``

The ``scope`` parameter is experimental. 

On Windows:  
``KEYEVENT_LOCAL`` will call ``SetWindowsHookEx`` with ``WH_GETMESSAGE`` which limits the event hook to the current thread.
 
``KEYEVENT_GLOBAL`` will call ``SetWindowsHookEx`` with ``WH_KEYBOARD_LL``.  

On Mac:  

``KEYEVENT_LOCAL`` will call ``- addLocalMonitorForEventsMatchingMask:``.

``KEYEVENT_GLOBAL`` will call ``- addGlobalMonitorForEventsMatchingMask``.

Either way, __you cannot modify or otherwise prevent the event from being delivered to its original target application__.

See source code for implementation.

### Signature of callback method

Parameter|Type|Description
------------|------------|----
keycode|LONGINT|Mac keycode or Windows virtual keycode if not mapped
context|TEXT|text passed to ``ON MEDIA KEY CALL``

### List of supported keycodes

Mac key code|mapped Windows virtual key code
------------|------------
KEYTYPE_BRIGHTNESS_DOWN|
KEYTYPE_BRIGHTNESS_UP|
KEYTYPE_CAPS_LOCK|VK_CAPITAL,VK_OEM_ATTN
KEYTYPE_CONTRAST_DOWN|
KEYTYPE_CONTRAST_UP|
KEYTYPE_DOWN_ARROW_KEY|
KEYTYPE_EJECT|
KEYTYPE_FAST|
KEYTYPE_HELP|VK_HELP
KEYTYPE_ILLUMINATION_DOWN|
KEYTYPE_ILLUMINATION_TOGGLE|
KEYTYPE_ILLUMINATION_UP|
KEYTYPE_LAUNCH_PANEL|
KEYTYPE_MUTE|VK_VOLUME_MUTE
KEYTYPE_NEXT|VK_MEDIA_NEXT_TRACK
KEYTYPE_NUM_LOCK|VK_NUMLOCK
KEYTYPE_PLAY|VK_MEDIA_PLAY_PAUSE
KEYTYPE_POWER_KEY|VK_SLEEP
KEYTYPE_PREVIOUS|VK_MEDIA_PREV_TRACK
KEYTYPE_REWIND|
KEYTYPE_SOUND_DOWN|VK_VOLUME_DOWN
KEYTYPE_SOUND_UP|VK_VOLUME_UP
KEYTYPE_UP_ARROW_KEY|
KEYTYPE_VIDMIRROR|
||VK_ACCEPT
||VK_APPS
||VK_BROWSER_BACK
||VK_BROWSER_FAVORITES
||VK_BROWSER_FORWARD
||VK_BROWSER_HOME
||VK_BROWSER_REFRESH
||VK_BROWSER_SEARCH
||VK_BROWSER_STOP
||VK_CANCEL
||VK_CLEAR
||VK_CONVERT
||VK_CRSEL
||VK_EREOF
||VK_EXECUTE
||VK_EXSEL
||VK_F16
||VK_F17
||VK_F18
||VK_F19
||VK_F20
||VK_F21
||VK_F22
||VK_F23
||VK_F24
||VK_FINAL
||VK_ICO_00
||VK_ICO_CLEAR
||VK_ICO_HELP
||VK_INSERT
||VK_JUNJA
||VK_KANA
||VK_KANJI
||VK_LAUNCH_APP1
||VK_LAUNCH_APP2
||VK_LAUNCH_MAIL
||VK_LAUNCH_MEDIA_SELECT
||VK_LCONTROL
||VK_LMENU
||VK_LSHIFT
||VK_LWIN
||VK_MEDIA_STOP
||VK_MODECHANGE
||VK_NONCONVERT
||VK_OEM_FJ_JISHO
||VK_NONAME
||VK_OEM_1
||VK_OEM_102
||VK_OEM_2
||VK_OEM_3
||VK_OEM_4
||VK_OEM_5
||VK_OEM_6
||VK_OEM_7
||VK_OEM_8
||VK_OEM_ATTN
||VK_OEM_AUTO
||VK_OEM_AX
||VK_OEM_BACKTAB
||VK_OEM_CLEAR
||VK_OEM_COMMA
||VK_OEM_COPY
||VK_OEM_CUSEL
||VK_OEM_ENLW
||VK_OEM_FINISH
||VK_OEM_FJ_LOYA
||VK_OEM_FJ_MASSHOU
||VK_OEM_FJ_ROYA
||VK_OEM_FJ_TOUROKU
||VK_OEM_JUMP
||VK_OEM_MINUS
||VK_OEM_PA1
||VK_OEM_PA2
||VK_OEM_PA3
||VK_OEM_PERIOD
||VK_OEM_PLUS
||VK_OEM_RESET
||VK_OEM_WSCTRL
||VK_PA1
||VK_PACKET
||VK_PAUSE
||VK_PLAY
||VK_PROCESSKEY
||VK_PRINT
||VK_RCONTROL
||VK_RMENU
||VK_RSHIFT
||VK_RWIN
||VK_SCROLL
||VK_SELECT
||VK_SEPARATOR
||VK_SNAPSHOT
||VK_ZOOM
