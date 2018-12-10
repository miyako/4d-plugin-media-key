/* --------------------------------------------------------------------------------
 #
 #	4DPlugin.cpp
 #	source generated by 4D Plugin Wizard
 #	Project : Media Key
 #	author : miyako
 #	2018/12/10
 #
 # --------------------------------------------------------------------------------*/


#include "4DPluginAPI.h"
#include "4DPlugin.h"

#define CALLBACK_IN_NEW_PROCESS 0
#define CALLBACK_SLEEP_TIME 59
#define CALLBACK_BY_UNFREEZE 1

#pragma mark -

void PluginMain(PA_long32 selector, PA_PluginParameters params)
{
	try
	{
		PA_long32 pProcNum = selector;
		sLONG_PTR *pResult = (sLONG_PTR *)params->fResult;
		PackagePtr pParams = (PackagePtr)params->fParameters;

		CommandDispatcher(pProcNum, pResult, pParams); 
	}
	catch(...)
	{

	}
}

void CommandDispatcher (PA_long32 pProcNum, sLONG_PTR *pResult, PackagePtr pParams)
{
	switch(pProcNum)
	{
        case kInitPlugin :
        case kServerInitPlugin :
            OnStartup();
            break;

        case kCloseProcess :
            OnCloseProcess();
            break;

        case kDeinitPlugin :
            OnExit();
            break;
            
// --- Media Key

		case 1 :
			ON_MEDIA_KEY_CALL(pResult, pParams);
			break;

	}
}

#pragma mark -

void generateUuid(C_TEXT &returnValue)
{
#if VERSIONWIN
	RPC_WSTR str;
	UUID uuid;

	if (UuidCreate(&uuid) == RPC_S_OK) {
		if (UuidToString(&uuid, &str) == RPC_S_OK) {
			size_t len = wcslen((const wchar_t *)str);

			std::vector<wchar_t>buf(len);
			memcpy(&buf[0], str, len * sizeof(wchar_t));
			_wcsupr((wchar_t *)&buf[0]);

			returnValue.setUTF16String((const PA_Unichar *)&buf[0], len);

			RpcStringFree(&str);
		}
	}
#else
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
	returnValue.setUTF16String([[[NSUUID UUID]UUIDString]stringByReplacingOccurrencesOfString:@"-" withString:@""]);
#else
	CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
	NSString *uuid_str = (NSString *)CFUUIDCreateString(kCFAllocatorDefault, uuid);
	returnValue.setUTF16String([uuid_str stringByReplacingOccurrencesOfString : @"-" withString:@""]);
#endif
#endif
}

#pragma mark -

// ----------------------------------- Media Key ----------------------------------

#include <mutex>

std::mutex globalMutex; /* for MEDIA_KEY_CODES */
std::mutex globalMutex1;/* for METHOD_PROCESS_ID */
std::mutex globalMutex2;/* for WATCH_METHOD,WATCH_CONTEXT,WATCH_SCOPE */
std::mutex globalMutex3;/* PROCESS_SHOULD_TERMINATE */
std::mutex globalMutex4;/* PROCESS_SHOULD_RESUME */

namespace MK
{
    //constants
    process_name_t MONITOR_PROCESS_NAME = (PA_Unichar *)"$\0m\0e\0d\0i\0a\0\0\0\0";
    process_stack_size_t MONITOR_PROCESS_STACK_SIZE = 0;
    
    //context management
    std::vector<media_key_t>MEDIA_KEY_CODES;
    C_TEXT WATCH_CONTEXT;
    media_key_scope_t WATCH_SCOPE;
    
    //callback management
    C_TEXT WATCH_METHOD;
    process_number_t METHOD_PROCESS_ID = 0;
    bool PROCESS_SHOULD_TERMINATE;
    bool PROCESS_SHOULD_RESUME = false;
}

bool IsProcessOnExit()
{
    C_TEXT name;
    PA_long32 state, time;
    PA_GetProcessInfo(PA_GetCurrentProcessNumber(), name, &state, &time);
    CUTF16String procName(name.getUTF16StringPtr());
    CUTF16String exitProcName((PA_Unichar *)"$\0x\0x\0\0\0");
    return (!procName.compare(exitProcName));
}

void OnStartup()
{
    
}

void OnExit()
{
    
}

void OnCloseProcess()
{
    if(IsProcessOnExit())
    {
        listenerLoopFinish();
    }
}

#pragma mark -

HHOOK eventMonitor = NULL;

#if VERSIONMAC
#if USE_NSEVENT
#else
static OSStatus onEventCall(EventHandlerCallRef inCaller, EventRef inEvent, void *inUserData)
{
    UInt32 keyCode;

    switch (GetEventClass(inEvent))
    {
        case kEventClassKeyboard :
            switch (GetEventKind(inEvent))
        {
            case kEventRawKeyDown:
                GetEventParameter(inEvent,
                                  kEventParamKeyCode,
                                  typeUInt32,
                                  NULL,
                                  sizeof(UInt32),
                                  NULL,
                                  &keyCode);
                
                NSLog(@"keyCode:%d", (unsigned int)keyCode);

                switch (keyCode)
            {
                case kVK_VolumeUp:
                    break;
                case kVK_VolumeDown:
                    break;
                case kVK_Mute:
                    break;
                case kVK_PageUp:
                    break;
                case kVK_PageDown:
                    break;
                case kVK_CapsLock:
                    break;
            }
                /* handle event */
                break;
        }
            break;
    }
    
    return eventNotHandledErr;
}
#endif
#else
LRESULT CALLBACK onEventCall(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION)
    {
        MSG* pMsg = (MSG*)lParam;
        if (pMsg->message == WM_KEYDOWN)
        {
            /*
             UINT keyCode = pMsg->wParam;
             BYTE keyboard[256];
             if (GetKeyboardState(keyboard))
             {
             WCHAR buf[5] = {};
             if (1 == ToUnicode(keycode, MapVirtualKey(keyCode, MAPVK_VK_TO_VSC), keyboard, buf, 4, 0))
             {
             
             }
             
             }
             */
            int keyCode = 0;
            switch (pMsg->wParam)
            {
                case VK_VOLUME_UP:
					keyCode = NX_KEYTYPE_SOUND_UP;
                    break;
                case VK_VOLUME_DOWN:
					keyCode = NX_KEYTYPE_SOUND_DOWN;
                    break;
//                case NX_KEYTYPE_BRIGHTNESS_UP:
//                case NX_KEYTYPE_BRIGHTNESS_DOWN:
				case VK_CAPITAL:
				case VK_OEM_ATTN: /* VMWare Fusion */
					keyCode = NX_KEYTYPE_CAPS_LOCK;
                    break;
                case VK_HELP:
					keyCode = NX_KEYTYPE_HELP;
                    break;
                case VK_SLEEP:
					keyCode = NX_POWER_KEY;/* power => sleep */
                    break;
                case VK_VOLUME_MUTE:
					keyCode = NX_KEYTYPE_MUTE;
                    break;
//                case VK_PRIOR:
//                case VK_NEXT:
                case VK_NUMLOCK:
					keyCode = NX_KEYTYPE_NUM_LOCK;
                    break;
//                case NX_KEYTYPE_CONTRAST_UP:
//                case NX_KEYTYPE_CONTRAST_DOWN:
//                case NX_KEYTYPE_LAUNCH_PANEL:
//                case NX_KEYTYPE_EJECT:
//                case NX_KEYTYPE_VIDMIRROR:
                case VK_MEDIA_PLAY_PAUSE:
					keyCode = NX_KEYTYPE_PLAY;
                    break;
                case VK_MEDIA_NEXT_TRACK:
					keyCode = NX_KEYTYPE_NEXT;
                    break;
                case VK_MEDIA_PREV_TRACK:
					keyCode = NX_KEYTYPE_PREVIOUS;
                    break;
//                case NX_KEYTYPE_FAST:
//                case NX_KEYTYPE_REWIND:
//                case NX_KEYTYPE_ILLUMINATION_UP:
//                case NX_KEYTYPE_ILLUMINATION_DOWN:
//                case NX_KEYTYPE_ILLUMINATION_TOGGLE:
				default:
					keyCode = (int)pMsg->wParam;
						break;
            }
			if (keyCode != 0)
			{
				std::lock_guard<std::mutex> lock(globalMutex);

				MK::MEDIA_KEY_CODES.push_back((media_key_t)keyCode);
			}
			listenerLoopExecute();
        }
    }
    
    return CallNextHookEx( eventMonitor, code, wParam, lParam );
}
#endif

#pragma mark -

void listenerLoopBegin()
{
#if VERSIONMAC
#if USE_NSEVENT
    /* cocoa */
    if(MK::WATCH_SCOPE == media_key_scope_local)
    {
        eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSSystemDefinedMask
                                                             handler:^(NSEvent *event)
                        {
                            
                            
                            int keyCode = (([event data1] & 0xFFFF0000) >> 16);
                            int keyFlags = ([event data1] & 0x0000FFFF);
                            int keyState = (((keyFlags & 0xFF00) >> 8)) == 0xA;/* 1:down, 0:up */
                            
                            if(keyState == 1)
                            {
                                switch (keyCode)
                                {
                                    case NX_KEYTYPE_SOUND_UP:
                                    case NX_KEYTYPE_SOUND_DOWN:
                                    case NX_KEYTYPE_BRIGHTNESS_UP:
                                    case NX_KEYTYPE_BRIGHTNESS_DOWN:
                                    case NX_KEYTYPE_CAPS_LOCK:
                                    case NX_KEYTYPE_HELP:
                                    case NX_POWER_KEY:
                                    case NX_KEYTYPE_MUTE:
                                    case NX_UP_ARROW_KEY:
                                    case NX_DOWN_ARROW_KEY:
                                    case NX_KEYTYPE_NUM_LOCK:
                                    case NX_KEYTYPE_CONTRAST_UP:
                                    case NX_KEYTYPE_CONTRAST_DOWN:
                                    case NX_KEYTYPE_LAUNCH_PANEL:
                                    case NX_KEYTYPE_EJECT:
                                    case NX_KEYTYPE_VIDMIRROR:
                                    case NX_KEYTYPE_PLAY:
                                    case NX_KEYTYPE_NEXT:
                                    case NX_KEYTYPE_PREVIOUS:
                                    case NX_KEYTYPE_FAST:
                                    case NX_KEYTYPE_REWIND:
                                    case NX_KEYTYPE_ILLUMINATION_UP:
                                    case NX_KEYTYPE_ILLUMINATION_DOWN:
                                    case NX_KEYTYPE_ILLUMINATION_TOGGLE:
                                        NSLog(@"keyCode:%d keyFlags:%d %d:keyState", keyCode, keyFlags, keyState);
                                        if(1)
                                        {
                                            std::lock_guard<std::mutex> lock(globalMutex);
                                            
                                            MK::MEDIA_KEY_CODES.push_back((media_key_t)keyCode);
                                        }
                                        listenerLoopExecute();
                                        break;
                                }
                            }

                            return event;
                        }];
        NSLog(@"%@", @"addLocalMonitorForEventsMatchingMask");
    }else
    {
        eventMonitor = [NSEvent addGlobalMonitorForEventsMatchingMask:NSSystemDefinedMask
                                                             handler:^(NSEvent *event)
                        {
                            
                            
                            int keyCode = (([event data1] & 0xFFFF0000) >> 16);
                            int keyFlags = ([event data1] & 0x0000FFFF);
                            int keyState = (((keyFlags & 0xFF00) >> 8)) == 0xA;/* 1:down, 0:up */
                            
                            if(keyState == 1)
                            {
                                switch (keyCode)
                                {
                                    case NX_KEYTYPE_SOUND_UP:
                                    case NX_KEYTYPE_SOUND_DOWN:
                                    case NX_KEYTYPE_BRIGHTNESS_UP:
                                    case NX_KEYTYPE_BRIGHTNESS_DOWN:
                                    case NX_KEYTYPE_CAPS_LOCK:
                                    case NX_KEYTYPE_HELP:
                                    case NX_POWER_KEY:
                                    case NX_KEYTYPE_MUTE:
                                    case NX_UP_ARROW_KEY:
                                    case NX_DOWN_ARROW_KEY:
                                    case NX_KEYTYPE_NUM_LOCK:
                                    case NX_KEYTYPE_CONTRAST_UP:
                                    case NX_KEYTYPE_CONTRAST_DOWN:
                                    case NX_KEYTYPE_LAUNCH_PANEL:
                                    case NX_KEYTYPE_EJECT:
                                    case NX_KEYTYPE_VIDMIRROR:
                                    case NX_KEYTYPE_PLAY:
                                    case NX_KEYTYPE_NEXT:
                                    case NX_KEYTYPE_PREVIOUS:
                                    case NX_KEYTYPE_FAST:
                                    case NX_KEYTYPE_REWIND:
                                    case NX_KEYTYPE_ILLUMINATION_UP:
                                    case NX_KEYTYPE_ILLUMINATION_DOWN:
                                    case NX_KEYTYPE_ILLUMINATION_TOGGLE:
                                        NSLog(@"keyCode:%d keyFlags:%d %d:keyState", keyCode, keyFlags, keyState);
                                        if(1)
                                        {
                                            std::lock_guard<std::mutex> lock(globalMutex);
                                            
                                            MK::MEDIA_KEY_CODES.push_back((media_key_t)keyCode);
                                        }
                                        listenerLoopExecute();
                                        break;
                                }
                            }
                            
                            return;
                        }];
        NSLog(@"%@", @"addGlobalMonitorForEventsMatchingMask");
    }
#else
    /* carbon */
    EventTypeSpec kKeyboardEvents[] = {
        {kEventClassKeyboard, kEventRawKeyDown}
    };
    EventHandlerUPP upp = NewEventHandlerUPP(onEventCall);
    InstallEventHandler(GetApplicationEventTarget(),
                        upp,
                        GetEventTypeCount(kKeyboardEvents),
                        kKeyboardEvents,
                        0, &eventMonitor);
    DisposeEventHandlerUPP(upp);
    NSLog(@"%@", @"InstallEventHandler");
#endif
#else
    /* windows */
    if(MK::WATCH_SCOPE == media_key_scope_local)
    {
        eventMonitor = SetWindowsHookEx(WH_GETMESSAGE,  (HOOKPROC)onEventCall, 0, GetCurrentThreadId());
    }else
    {
        eventMonitor = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)onEventCall, (HINSTANCE)GetModuleHandle(0), 0);
    }
#endif
}

void listenerLoopEnd()
{
#if VERSIONMAC
#if USE_NSEVENT
    /* cocoa */
    [NSEvent removeMonitor:eventMonitor];
    NSLog(@"%@", @"removeMonitor");
#else
    /* carbon */
    RemoveEventHandler(eventMonitor);
    NSLog(@"%@", @"RemoveEventHandler");
#endif
#else
    /* windows */
    UnhookWindowsHookEx(eventMonitor);
#endif
    eventMonitor = NULL;
}

void listenerLoop()
{
#if VERSIONMAC
     PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopBegin, NULL);
#else
    listenerLoopBegin();
#endif

    if(1)
    {
        std::lock_guard<std::mutex> lock(globalMutex3);
        
        MK::PROCESS_SHOULD_TERMINATE = false;
    }
    
    /* Current process returns 0 for PA_NewProcess */
    PA_long32 currentProcessNumber = PA_GetCurrentProcessNumber();
    
    while(!PA_IsProcessDying())
    {
//        PA_YieldAbsolute();
        
        bool PROCESS_SHOULD_RESUME;
        bool PROCESS_SHOULD_TERMINATE;
        
        if(1)
        {
            PROCESS_SHOULD_RESUME = MK::PROCESS_SHOULD_RESUME;
            PROCESS_SHOULD_TERMINATE = MK::PROCESS_SHOULD_TERMINATE;
        }
        
        if(PROCESS_SHOULD_RESUME)
        {
            size_t count_media_keys;
            
            if(1)
            {
                std::lock_guard<std::mutex> lock(globalMutex);
                
                count_media_keys = MK::MEDIA_KEY_CODES.size();
            }
            
            while(count_media_keys)
            {
//                PA_YieldAbsolute();
                
                if(CALLBACK_IN_NEW_PROCESS)
                {
                    C_TEXT processName;
                    generateUuid(processName);
                    PA_NewProcess((void *)listenerLoopExecuteMethod,
                                  MK::MONITOR_PROCESS_STACK_SIZE,
                                  (PA_Unichar *)processName.getUTF16StringPtr());
                }else
                {
                    listenerLoopExecuteMethod();
                }
                
                if(PROCESS_SHOULD_TERMINATE)
                    break;
                
                if(1)
                {
                    std::lock_guard<std::mutex> lock(globalMutex);
                    
                    count_media_keys = MK::MEDIA_KEY_CODES.size();
                    PROCESS_SHOULD_TERMINATE = MK::PROCESS_SHOULD_TERMINATE;
                }
            }
            
            if(1)
            {
                std::lock_guard<std::mutex> lock(globalMutex4);
                
                MK::PROCESS_SHOULD_RESUME = false;
            }
            
        }else
        {
#if CALLBACK_BY_UNFREEZE
            PA_FreezeProcess(currentProcessNumber);
#else
            PA_PutProcessToSleep(currentProcessNumber, CALLBACK_SLEEP_TIME);
#endif
        }
        
        if(1)
        {
            PROCESS_SHOULD_TERMINATE = MK::PROCESS_SHOULD_TERMINATE;
        }
        
        if(PROCESS_SHOULD_TERMINATE)
            break;
        
    }
    
#if VERSIONMAC
    PA_RunInMainProcess((PA_RunInMainProcessProcPtr)listenerLoopEnd, NULL);
#else
    listenerLoopEnd();
#endif

    if(1)
    {
        std::lock_guard<std::mutex> lock(globalMutex);
        
        MK::MEDIA_KEY_CODES.clear();
    }
    
    if(1)
    {
        std::lock_guard<std::mutex> lock(globalMutex1);
        
        MK::METHOD_PROCESS_ID = 0;
    }
    
    PA_KillProcess();
}

void listenerLoopStart()
{
    if (!MK::METHOD_PROCESS_ID)
    {
        std::lock_guard<std::mutex> lock(globalMutex1);
        
        MK::METHOD_PROCESS_ID = PA_NewProcess((void *)listenerLoop,
                                               MK::MONITOR_PROCESS_STACK_SIZE,
                                               MK::MONITOR_PROCESS_NAME);
    }
    else
    {
        listenerLoopFinish();
    }

}

void listenerLoopFinish()
{
    if(MK::METHOD_PROCESS_ID)
    {
        if(1)
        {
            std::lock_guard<std::mutex> lock(globalMutex3);
            
            MK::PROCESS_SHOULD_TERMINATE = true;
        }
        
//        PA_YieldAbsolute();
        
        if(1)
        {
            std::lock_guard<std::mutex> lock(globalMutex4);
            
            MK::PROCESS_SHOULD_RESUME = true;
        }
#if CALLBACK_BY_UNFREEZE
        PA_UnfreezeProcess(MK::METHOD_PROCESS_ID);
#endif
    }
}

void listenerLoopExecute()
{
    if(1)
    {
        std::lock_guard<std::mutex> lock(globalMutex3);
        
        MK::PROCESS_SHOULD_TERMINATE = false;
    }
    
    if(1)
    {
        std::lock_guard<std::mutex> lock(globalMutex4);
        
        MK::PROCESS_SHOULD_RESUME = true;
    }
#if CALLBACK_BY_UNFREEZE
    PA_UnfreezeProcess(MK::METHOD_PROCESS_ID);
#endif
}

void listenerLoopExecuteMethod()
{
    media_key_t media_key;
    
    if(1)
    {
        std::lock_guard<std::mutex> lock(globalMutex);
        
        std::vector<media_key_t>::iterator p = MK::MEDIA_KEY_CODES.begin();
        
        media_key = *p;
        
        MK::MEDIA_KEY_CODES.erase(p);
    }
    
    method_id_t methodId = PA_GetMethodID((PA_Unichar *)MK::WATCH_METHOD.getUTF16StringPtr());
    
    if(methodId)
    {
        PA_Variable    params[2];
        params[0] = PA_CreateVariable(eVK_Longint);
        params[1] = PA_CreateVariable(eVK_Unistring);
        
        PA_Unistring context = PA_CreateUnistring((PA_Unichar *)MK::WATCH_CONTEXT.getUTF16StringPtr());
        
        PA_SetLongintVariable(&params[0], media_key);
        PA_SetStringVariable(&params[1], &context);
        
        PA_ExecuteMethodByID(methodId, params, 2);
        
        PA_ClearVariable(&params[0]);
        PA_ClearVariable(&params[1]);
        
    }else
    {
        PA_Variable    params[3];
        params[1] = PA_CreateVariable(eVK_Unistring);
        params[2] = PA_CreateVariable(eVK_Unistring);
        
        PA_Unistring context = PA_CreateUnistring((PA_Unichar *)MK::WATCH_CONTEXT.getUTF16StringPtr());
        
        PA_SetLongintVariable(&params[1], media_key);
        PA_SetStringVariable(&params[2], &context);
        
        params[0] = PA_CreateVariable(eVK_Unistring);
        PA_Unistring method = PA_CreateUnistring((PA_Unichar *)MK::WATCH_METHOD.getUTF16StringPtr());
        PA_SetStringVariable(&params[0], &method);
        
        PA_ExecuteCommandByID(1007, params, 2);
        
        PA_ClearVariable(&params[0]);
        PA_ClearVariable(&params[1]);
        PA_ClearVariable(&params[2]);
    }
}

#pragma mark -

void ON_MEDIA_KEY_CALL(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT Param1;
	C_TEXT Param2;
    C_LONGINT Param3;
    
	Param1.fromParamAtIndex(pParams, 1);
	Param2.fromParamAtIndex(pParams, 2);
    Param3.fromParamAtIndex(pParams, 3);
    
    if(1)
    {
        std::lock_guard<std::mutex> lock(globalMutex2);
        
        if(!Param1.getUTF16Length())
        {
            MK::WATCH_METHOD.setUTF16String((PA_Unichar *)"\0\0", 0);
        }else{
            MK::WATCH_METHOD.setUTF16String(Param1.getUTF16StringPtr(), Param1.getUTF16Length());
            MK::WATCH_SCOPE = (media_key_scope_t)Param3.getIntValue();
        }
        
        if(!Param2.getUTF16Length())
        {
            MK::WATCH_CONTEXT.setUTF16String((PA_Unichar *)"\0\0", 0);
        }else{
            MK::WATCH_CONTEXT.setUTF16String(Param2.getUTF16StringPtr(), Param2.getUTF16Length());
            MK::WATCH_SCOPE = (media_key_scope_t)Param3.getIntValue();
        }
    }

    listenerLoopStart();
}
