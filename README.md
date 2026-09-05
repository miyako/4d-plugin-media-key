# 4d-plugin-media-key

Captures OS-level media-key presses (volume, mute, play/pause, brightness, and similar keyboard keys not normally readable from 4D) and forwards them to a 4D project method. On macOS it uses a Cocoa `NSEvent` monitor (`addLocalMonitorForEventsMatchingMask:`/`addGlobalMonitorForEventsMatchingMask:`); on Windows it installs a `SetWindowsHookEx` hook (`WH_GETMESSAGE` or `WH_KEYBOARD_LL`). The plugin never consumes the event — whichever application the key was actually meant for still receives it as normal.

| Command | Returns | Purpose |
|---|---|---|
| [ON MEDIA KEY CALL](#on-media-key-call) | *(none)* | Registers (or stops) a project method that's called whenever a media key is pressed. |

**Platforms:** macOS 10.x+ (64-bit, Cocoa builds only) · Windows (32-bit and 64-bit)

---

## Requirements & platform notes

- **The command has no return value** — the manifest declares it with no result-type token, and the implementation never signals success or failure back to 4D. If your callback never fires, there's no 4D-level error to catch; see [Error handling](#error-handling--troubleshooting).
- **Only one listener at a time.** Calling `ON MEDIA KEY CALL` again while a listener is already active does **not** swap in the new method — it stops the current listener instead. To change the callback while already listening, call once with an empty method name to stop, then call again with the new method. This is current, verified behavior, not a documented "feature" — treat it as a hard constraint until/unless the plugin is changed.
- **Scope values aren't shipped as named 4D constants.** The plugin's own README refers to the third parameter using the names `KEYEVENT_LOCAL`/`KEYEVENT_GLOBAL`, but the plugin's manifest declares no constants for 4D, and the underlying C++ enum (`media_key_scope_local = 0`, `media_key_scope_global = 1`) is internal to the plugin, not exposed to 4D code. Pass the raw Longint (`0` or `1`) unless you've separately declared those names as constants in your own project.
- **Same caution applies to keycode names.** The keycode your callback method receives is a raw Longint matching either a macOS key-type value or (on Windows) a mapped `NX_KEYTYPE_*`-style value — see the [keycode table](#keycode-reference) below. Symbolic names like `KEYTYPE_PLAY` are documentation labels for those numbers, not confirmed-available 4D constants; declare your own local constants if you want named comparisons.
- **`scope` is read on every call, even though it's sometimes described as optional.** The compiled command syntax (`ON MEDIA KEY CALL(&T;&T;&L)`) declares all three parameters, and the implementation reads all three unconditionally each time the command runs. Pass an explicit `0` (local) if you don't need global capture rather than omitting the parameter.
- **Windows: this installs a system-wide hook when using global scope.** `WH_KEYBOARD_LL` callbacks are subject to an OS-enforced timeout; slow or blocking code in your callback method risks Windows silently disabling the hook, or a perceptible system-wide input lag while it runs. Keep the callback method fast — hand off any real work to be done asynchronously rather than doing it inline.
- **macOS: global capture may prompt for system permission** on newer macOS versions (Accessibility / Input Monitoring), depending on the OS version you're targeting — if the callback silently never fires under global scope, check System Settings → Privacy & Security first.
- The manifest marks this command **not thread-safe** (`"threadSafe": false`) — don't call it concurrently from multiple 4D processes and expect both calls to "win."
- No Linux support — Mac and Windows only (confirmed by the plugin's platform branches; there is no third platform code path).

---

## ON MEDIA KEY CALL

### Syntax

```
ON MEDIA KEY CALL ( method ; context ; scope )
```

No return value.

| Parameter | Type | Description |
|---|---|---|
| `method` | Text | Name of the 4D project method to call on every media-key press. Pass an empty string to stop listening — but only when a listener is currently active (see [Error handling](#error-handling--troubleshooting)). |
| `context` | Text | Arbitrary text passed through unchanged to your callback method's second parameter. Use it to carry state (a window reference, a mode flag, etc.) into the callback. |
| `scope` | Longint | `0` for local capture (your process/thread only), `1` for global capture (system-wide, even when 4D isn't the active app). Documented by the plugin's README as experimental. |
| Result | — | No return value. |

### Description

Registering a callback starts a dedicated 4D background process that installs the platform hook and waits for events; it's created once, the first time `ON MEDIA KEY CALL` is called with a non-empty method, and torn down when you call it again with an empty method.

**On Windows**, `scope = 0` installs `WH_GETMESSAGE`, limiting capture to the current thread; `scope = 1` installs `WH_KEYBOARD_LL`, a global low-level keyboard hook. A number of standard keys are mapped to the plugin's own cross-platform codes (`NX_KEYTYPE_SOUND_UP`, `NX_KEYTYPE_MUTE`, `NX_KEYTYPE_PLAY`, `NX_KEYTYPE_NEXT`, `NX_KEYTYPE_PREVIOUS`, `NX_KEYTYPE_CAPS_LOCK`, `NX_KEYTYPE_HELP`, `NX_POWER_KEY`, `NX_KEYTYPE_NUM_LOCK`); any other recognized Windows virtual-key code (browser keys, launch keys, F16–F24, IME keys, etc. — see the [keycode table](#keycode-reference)) is passed through as its raw `VK_*` value instead of being remapped.

**On macOS**, `scope = 0` calls `addLocalMonitorForEventsMatchingMask:`, `scope = 1` calls `addGlobalMonitorForEventsMatchingMask:`, both watching for `NSSystemDefinedMask` events. The keycode delivered to your method is the raw macOS key-type value (`NX_KEYTYPE_*`/`KEYTYPE_*` in Apple's headers) with no remapping.

Either way, the underlying OS still delivers the key event to whatever application it was actually intended for — you cannot suppress or modify delivery from this plugin.

Your project method (named by `method`) is called with two parameters:

```4d
 // $1 : Longint — the keycode (see keycode reference below)
 // $2 : Text    — the context text you passed to ON MEDIA KEY CALL
```

### Keycode reference

Common keys and their delivered value (macOS key-type name, and the Windows virtual key it's mapped from — where the plugin remaps one):

| Meaning | macOS value | Mapped from (Windows) |
|---|---|---|
| Volume up | `KEYTYPE_SOUND_UP` | `VK_VOLUME_UP` |
| Volume down | `KEYTYPE_SOUND_DOWN` | `VK_VOLUME_DOWN` |
| Mute | `KEYTYPE_MUTE` | `VK_VOLUME_MUTE` |
| Play/Pause | `KEYTYPE_PLAY` | `VK_MEDIA_PLAY_PAUSE` |
| Next track | `KEYTYPE_NEXT` | `VK_MEDIA_NEXT_TRACK` |
| Previous track | `KEYTYPE_PREVIOUS` | `VK_MEDIA_PREV_TRACK` |
| Caps Lock | `KEYTYPE_CAPS_LOCK` | `VK_CAPITAL`, `VK_OEM_ATTN` |
| Num Lock | `KEYTYPE_NUM_LOCK` | `VK_NUMLOCK` |
| Help | `KEYTYPE_HELP` | `VK_HELP` |
| Power/Sleep | `KEYTYPE_POWER_KEY` | `VK_SLEEP` |
| Brightness up/down | `KEYTYPE_BRIGHTNESS_UP` / `_DOWN` | *(no Windows equivalent)* |
| Eject | `KEYTYPE_EJECT` | *(no Windows equivalent)* |

On Windows, many other keys (browser navigation, launch keys, F16–F24, IME/Kana/Kanji keys, and others) are recognized and passed through as their raw `VK_*` numeric value rather than remapped — see the plugin's README for the full list if you need to branch on one of these.

### Example

No sample/test `.4dm` method was included with this plugin's source, so the following is an illustrative example built from standard 4D language constructs, not a verbatim provided sample.

Register a global listener, typically from your startup/on-startup method:

```4d
 // On Startup, or a method run once at app launch
ON MEDIA KEY CALL("MediaKey_Callback";"main window";1)  // 1 = global
```

The callback method (`MediaKey_Callback`):

```4d
C_LONGINT($1)
C_TEXT($2)
C_LONGINT($keyCode)
C_TEXT($context)

$keyCode:=$1
$context:=$2

Case of
    : ($keyCode=16)  //  NX_KEYTYPE_PLAY / KEYTYPE_PLAY
        ALERT("Play/Pause pressed ("+$context+")")
    : ($keyCode=0)  //  NX_KEYTYPE_SOUND_UP / KEYTYPE_SOUND_UP
        ALERT("Volume up ("+$context+")")
    : ($keyCode=7)  //  NX_KEYTYPE_MUTE / KEYTYPE_MUTE
        ALERT("Mute toggled ("+$context+")")
    Else 
        ALERT("Unhandled media key: "+String($keyCode)+" ("+$context+")")
End case 
```

Stop listening (only valid while a listener is already active — see [Error handling](#error-handling--troubleshooting)):

```4d
ON MEDIA KEY CALL("";"";0)
```

---

## Error handling & troubleshooting

- **Re-registering doesn't update an active listener.** Calling `ON MEDIA KEY CALL` with a new method while one is already running just stops the current listener; the new method/context you passed are stored but never picked up by a running process. Always stop (empty method) before re-registering with different parameters.
- **Calling with an empty method when nothing is listening still starts an idle background process**, rather than being a harmless no-op — because the "already running?" check and the "start vs. stop" branch share the same code path. Only send an empty `method` after you've actually registered a non-empty one.
- **No 4D error is ever raised** by this command — hook-install failures, an unresolved method name, or a missing OS permission all fail silently from 4D's point of view. If your callback never fires, check the platform-specific causes below rather than expecting an error in your method or the execution log.
- **On macOS, global scope can be blocked by system privacy settings** on newer OS versions; if global (`scope=1`) never fires but local (`scope=0`) does, check Accessibility / Input Monitoring permissions for the host app.
- **On Windows, a slow callback method under global scope can affect the whole system**, not just 4D — `WH_KEYBOARD_LL` hooks have to return quickly or Windows may disable the hook (and in the meantime, other applications' keyboard input can lag). Keep the project method's work minimal; defer anything heavier.
- **The original application still receives the key press regardless of what your callback does.** This plugin only observes events; it cannot consume, block, or remap them at the OS level.
- **Unmapped Windows keys are passed through as raw `VK_*` values**, not translated to a macOS-style constant — if you're branching on keycodes and want cross-platform parity, you'll need to handle both numbering schemes explicitly (see the [keycode table](#keycode-reference)).

---

## Quick reference

```4d
 // Start listening globally
ON MEDIA KEY CALL("MediaKey_Callback";"main window";1)

 // Stop listening (only while a listener is active)
ON MEDIA KEY CALL("";"";0)

 // MediaKey_Callback project method
C_LONGINT($1)
C_TEXT($2)
Case of
    : ($1=16)  // Play/Pause
         // ... handle play/pause ...
    : ($1=0)   // Volume up
         // ... handle volume up ...
    Else 
         // ... unhandled keycode $1, context $2 ...
End case 
```
