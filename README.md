# 4d-plugin-media-key
Capture media key press events

### Platform

| carbon | cocoa | win32 | win64 |
|:------:|:-----:|:---------:|:---------:|
||<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|

__Mac version is 64-bit only!__ 

### Version

<img src="https://cloud.githubusercontent.com/assets/1725068/18940648/2192ddba-8645-11e6-864d-6d5692d55717.png" width="32" height="32" /> <img src="https://user-images.githubusercontent.com/1725068/41266195-ddf767b2-6e30-11e8-9d6b-2adf6a9f57a5.png" width="32" height="32" />

![preemption xx](https://user-images.githubusercontent.com/1725068/41327179-4e839948-6efd-11e8-982b-a670d511e04f.png)

## Syntax

```
ON MEDIA KEY CALL (method;context{;scope})
```

Parameter|Type|Description
------------|------------|----
method|TEXT|project method
context|TEXT|any text
scope|LONGINT|``KEYEVENT_LOCAL`` or ``KEYEVENT_GLOBAL``

### Signature of callback method

Parameter|Type|Description
------------|------------|----
keycode|LONGINT|Mac keycode 
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
