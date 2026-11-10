# 4d-plugin-media-key

Captures OS-level media-key presses (volume, mute, play/pause, brightness, and similar keyboard keys not normally readable from 4D) and forwards them to a 4D project method. On macOS it uses a Cocoa `NSEvent` monitor (`addLocalMonitorForEventsMatchingMask:`/`addGlobalMonitorForEventsMatchingMask:`); on Windows it installs a `SetWindowsHookEx` hook (`WH_GETMESSAGE` or `WH_KEYBOARD_LL`). The plugin never consumes the event — whichever application the key was actually meant for still receives it as normal.

## Native string ownership maintenance

The audit baseline is `3e567ec31cff0f15aacea5411479676c13e11e75`.
Both source and bundled manifests declare only `ON MEDIA KEY CALL(&T;&T;&L)`,
dispatched as command 1. Inventory covered `4DPlugin.cpp`, all 16 first-party
helper implementations and their headers, the bundled API implementation,
and Xcode plus both Visual Studio projects. All helpers are compiled by those
projects; no third-party library was changed.

| Surface | Reachability and ownership conclusion |
|---|---|
| Command, listener, UUID and callbacks (`4DPlugin.cpp`) | Active. Input text is copied into C_TEXT snapshots. Synchronous callback text now uses PA_SetUnistring on its existing owner, avoiding allocated-empty descriptor overwrites. The fallback key is initialized as Longint. The direct method's owning return is cleared independently of its arguments. |
| `C_TEXT` | Active. Native strings copy UTF-16 content; parameter/result setters use the engine content setter, not descriptor transfer. Platform conversion and Objective-C path utilities were inspected. |
| `ARRAY_TEXT` | Compiled, not reached by the dispatcher. Borrowed array content is copied into native strings; each output element receives a separately created allocation. Accepted insertions transfer ownership and are not immediately disposed. No array fix was needed for successful allocation/resize paths. |
| `C_PICTURE` / `CPicture` | Compiled, not reached by the dispatcher. The file-path argument now has one owner and one cleanup, rather than descriptor disposal plus variable cleanup. Conversion byte lengths are divided by sizeof(PA_Unichar) before constructing UTF-16 strings; the format argument updates its existing owner. The path result is retained before clearing its owning variable, and createCopy initializes its picture argument as Undefined before handing off an existing retained picture. |
| `C_POINTER` | Compiled, not reached by the dispatcher. getVariable copies definition metadata from PA_GetPointerValue; clearing that value is deferred because metadata-versus-owning-value semantics are ambiguous despite the API's general cleanup comment. |
| `C_BLOB`, `C_DATE`, `C_INTEGER`, `C_LONGINT`, `C_REAL`, `C_TIME`; `ARRAY_BOOLEAN`, `ARRAY_DATE`, `ARRAY_INTEGER`, `ARRAY_LONGINT`, `ARRAY_REAL`, `ARRAY_TIME` | Compiled, not reached by the dispatcher. Inspected scalar/array parameter and return helpers plus blob text conversion. No additional confirmed SDK string descriptor lifetime defect. |

Contracts come from this repository's unchanged SDK, not a release inferred
from its labels (header `v13`, implementation `v14`). SDK SHA-256:
`4DPluginAPI.c`: `2721c3d4338cc2a350599ffcee2a8008abaa756acfd45dd710720c64ca8c8ce3`;
`4DPluginAPI.h`: `7391841f0046f9e822b65ca6b07cafb3565aecfb58f65be4e0661adf97b8c786`.
The canonical guidance used was
[miyako/skills at merged PR 71](https://github.com/miyako/skills/blob/e6b492fcb81e5e1a7f7db723594b420e63bf6eb9/4d-skills/skills/4dplugin/SKILL.md).

PA_CreateVariable(Unistring) creates an empty owned string;
PA_SetStringVariable shallowly overwrites its descriptor. PA_SetUnistring
updates content through EX_SET_UNISTRING instead. PA_GetStringInArray is
borrowed; PA_SetStringInArray disposes the old element and transfers the
incoming descriptor only for accepted type/data/index/non-null-input
conditions (including index zero). PA_ExecuteMethodByID returns a separate
variable, including when its content copies an input. EXECUTE METHOD (1007)
and the picture commands used here are procedures, not owning function
results. This SDK has no object/collection property helper surfaces.

Run `python3 .github/tests/run_ownership.py` on macOS with Apple clang and
the system SDK. It compiles actual `4DPlugin.cpp`, all helpers and the full
unchanged bundled API against an EngineBlock callback shim, with ASan/UBSan.
No Xcode build phase, signing, packaging, notarization or publication runs.
The Native ownership workflow runs the same harness and existing lifecycle
tests on pull requests and relevant master changes.

The six harness cases cover callback arguments and independent/copied-input
returns, method/command handle allocation failure and host error cleanup,
picture path replacement/retention, exact ASCII/BMP/surrogate-pair/empty
format content, byte capacities and termination, borrowed array copies,
independent transfers, replacement/shrink/clear, and rejected type/index/
null-data/null-input assignments. Both allocated and null empty string
representations are exercised. SDK string/handle allocations and picture
references are counted because LeakSanitizer is not used on macOS.

To compare an immutable checkout, pass `--source-root /path/to/checkout`;
the current harness is still used with that checkout's complete sources and
SDK. At the baseline above, callbacks, callback-results and callback-errors
fail allocation accounting, path fails double-disposal accounting, and
conversion fails ASan with a heap-buffer-overflow at C_PICTURE.cpp:249;
arrays pass unchanged. Fixed sources pass all six cases (320 normal
callbacks, four host-error callbacks, four rejected handle allocations,
80 path loads and 80 conversions) and the existing 45 lifecycle tests.

This is host-shim evidence on macOS arm64, not certification in 4D or on
Windows. OS hooks, asynchronous cancellation/process abort, allocation
failure inside string creation or array resize, and nonempty picture
representation enumeration are not simulated. PA_GetPictureData's returned
type-string lifetime needs host evidence before cleanup changes. Unrelated
queue iterator/reentrancy and picture-only helper ownership paths are not
redesigned by this string audit. Tracked prebuilt binaries remain unchanged.

## Manual version and publication workflows

Run **Bump version** on the explicitly selected branch with `patch`, `minor`, or
`major`. It validates strict source `X.Y.Z` and commits only the Xcode
marketing/current-project versions, root Info.plist release fields and copied
English localized release display values. Supported Xcode version references
are preserved; absent CFBundleVersion remains absent. Prebuilt bundles,
localization revision counters, dependencies, manifest and 4D/schema metadata
are excluded. Bumping creates no tag and does not trigger Build.

The existing authority is **1.0.3** (six MARKETING_VERSION settings and root
CFBundleShortVersionString). The old bump header promised coordination with
CURRENT_PROJECT_VERSION, but history from the first 1.0.1 bump (`f0ecb5e`)
through 1.0.3 (`ddec223`) changed only marketing values. This migration repairs
six stale 1.0.0 build settings and the Resources phase's English 1.2 short/info
overrides to the existing 1.0.3, without a release increment. Copyright year,
source encodings/permissions and independent metadata are retained.

Bump checks a clean immutable selected checkout and the exact remote branch
SHA before edits and again before push. Its mandatory executable pre-push
guard checks receive-pack's advertised old SHA, exact target and new commit;
late advance, rewind, deletion and same-result races fail closed. Missing or
nonexecutable guards fail before edits. Push is nonforce, to only the selected
branch, with both `push.followTags=false` and `--no-follow-tags`. If the branch
moves, select its new state and dispatch again; nothing is force-updated.

Run **Build** separately on the desired branch/commit. It has zero dispatch
inputs; every checkout uses the selected immutable `github.sha`. It derives
the version from source, retains Windows x64/MSBuild v143 and macOS universal
builds, artifact merge, strict signature verification, ZIP/DMG notarization
and DMG stapling. Assets remain `Media Key-X.Y.Z.zip` and
`Media Key-X.Y.Z.dmg`. There is no new platform or default-branch-only gate.

The Xcode target also retains its existing active legacy ShellScript phase
(Developer ID signing and `altool` notarization using `@keychain:altool`).
This migration does not bypass, modernize or execute that phase. Its legacy
tool/credential requirements may still block a native Build independently of
the workflow's later signing/notarytool gates; lifecycle checks do not claim
native build success.

Publication is serialized repository-wide across branches. Before native work
and again after all build/sign/notary/staple gates, remote tags and authenticated
paginated releases (including drafts) must be absent for `vX.Y.Z`. Every page
and entry is validated; API 404/auth/network/malformed/later-page failures stop
the run rather than being treated as absence. Publication creates a tag by
POST at exactly the selected SHA (same-SHA creation races also fail), verifies
its remote SHA, then separately POSTs a public non-prerelease with explicit
tag/target/name and the original empty, non-generated notes policy. Asset POSTs
target only that newly returned release ID.

**Partial publication recovery:** there is no update, deletion, rollback,
retry or asset clobber. If tag creation, release creation, verification or an
upload fails, any accepted tag, release and assets remain. Inspect the error
and remote state manually before deciding on recovery; do not rerun Build for
an occupied version. A human must explicitly decide whether to finish the
retained publication or prepare a new source version. Bump may likewise leave
an unpushed local commit on failure; inspect it and redispatch at the current
branch state.

Local lifecycle checks (no native/signing execution):
`PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s .github/tests -q`
and `actionlint .github/workflows/*.yml`.

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
