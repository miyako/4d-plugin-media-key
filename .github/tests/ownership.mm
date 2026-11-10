#include "4DPlugin.cpp"
#include "PrivateTypes.h"
#include "EntryPoints.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>

namespace {
std::set<PA_Unichar*> strings;
std::map<PA_Handle, size_t> handles;
std::map<PA_Picture, unsigned> pictures;
bool allocatedEmpty = true;
bool directMethod = true;
bool failNextHandle = false;
bool callbackError = false;
int returnKind = 0;
unsigned calls = 0;
CUTF16String expectedContext, expectedMethod, expectedFormat;

void require(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "ownership assertion: %s\n", message);
        std::abort();
    }
}

PA_Unistring makeString(const PA_Unichar* input)
{
    size_t length = 0;
    while (input && input[length]) ++length;
    PA_Unistring result = {};
    result.fLength = static_cast<PA_long32>(length);
    if (length || allocatedEmpty) {
        result.fString = new PA_Unichar[length + 1];
        if (length) std::memcpy(result.fString, input, length * sizeof(PA_Unichar));
        result.fString[length] = 0;
        PA_Unichar* storage = result.fString;
        require(strings.insert(storage).second, "unique string allocation");
    }
    return result;
}

void disposeString(PA_Unistring value)
{
    if (value.fString) {
        PA_Unichar* storage = value.fString;
        require(strings.erase(storage) == 1, "string disposed twice or borrowed");
        delete[] value.fString;
    }
}

void checkString(PA_Unistring value, const CUTF16String& expected)
{
    require(value.fLength == expected.size(), "exact UTF-16 code-unit count");
    PA_Unichar* storage = value.fString;
    require(!storage || strings.count(storage), "live string allocation");
    require(value.fString || expected.empty(), "nonempty string has storage");
    if (value.fString) {
        require(value.fString[value.fLength] == 0, "terminator outside content");
        require(CUTF16String(value.fString, value.fLength) == expected, "exact UTF-16 content");
    }
}

PA_Picture newPicture()
{
    PA_Picture p = new char;
    pictures[p] = 1;
    return p;
}

void releasePicture(PA_Picture p)
{
    if (!p) return;
    require(pictures.count(p), "picture must be live");
    if (--pictures[p] == 0) {
        pictures.erase(p);
        delete static_cast<char*>(p);
    }
}

void clear(PA_Variable* variable)
{
    switch (variable->fType) {
        case eVK_Unistring:
            disposeString(variable->uValue.fString);
            break;
        case eVK_Picture:
            releasePicture(variable->uValue.fPicture);
            break;
        case eVK_ArrayUnicode: {
            PA_Handle h = variable->uValue.fArray.fData;
            if (h) {
                require(handles.count(h), "array handle live");
                PA_Unistring* data = reinterpret_cast<PA_Unistring*>(*h);
                for (PA_long32 i = 0; i <= variable->uValue.fArray.fNbElements; ++i)
                    disposeString(data[i]);
                std::free(*h);
                delete h;
                handles.erase(h);
            }
            break;
        }
        case eVK_Undefined:
        case eVK_Longint:
            break;
        default:
            require(false, "unmodeled variable cleanup");
    }
    *variable = {};
    variable->fType = eVK_Undefined;
}

PA_Variable callbackResult(PA_Unistring context)
{
    PA_Variable result = {};
    result.fType = eVK_Undefined;
    if (returnKind) {
        result.fType = eVK_Unistring;
        const PA_Unichar independent[] = {'r', 0x03a9, 0xd83d, 0xde00, 0};
        result.uValue.fString = makeString(returnKind == 1 ? independent : context.fString);
        require(!result.uValue.fString.fString || result.uValue.fString.fString != context.fString,
                "callback result is independently owned even when copying input");
    }
    return result;
}

void callback(PA_Variable* key, PA_Variable* context)
{
    require(key->fType == eVK_Longint && key->uValue.fLongint == NX_KEYTYPE_PLAY,
            "callback key and type preserved");
    require(context->fType == eVK_Unistring, "callback context type");
    checkString(context->uValue.fString, expectedContext);
    ++calls;
}

void engine(short selector, EngineBlock* eb)
{
    switch (selector) {
        case EX_CREATE_UNISTRING:
            eb->fUniString1 = makeString(reinterpret_cast<PA_Unichar*>(eb->fHandle));
            return;
        case EX_SET_UNISTRING: {
            // Copy first: the content setter may be passed storage from its old owner.
            CUTF16String copy(reinterpret_cast<PA_Unichar*>(eb->fHandle));
            disposeString(eb->fUniString1);
            eb->fUniString1 = makeString(copy.c_str());
            return;
        }
        case EX_DISPOSE_UNISTRING:
            disposeString(eb->fUniString1);
            return;
        case EX_CLEAR_VARIABLE:
            clear(static_cast<PA_Variable*>(eb->fPtr1));
            return;
        case EX_HANDLE_MANAGER: {
            PA_Handle h = eb->fParam1 == 1 ? nullptr : eb->fHandle;
            switch (eb->fParam1) {
                case 1:
                    if (failNextHandle) {
                        failNextHandle = false;
                        eb->fHandle = nullptr;
                        eb->fError = eER_NotEnoughMemory;
                        return;
                    }
                    h = new char*;
                    *h = static_cast<char*>(std::calloc(1, eb->fParam2));
                    handles[h] = eb->fParam2;
                    eb->fHandle = h;
                    break;
                case 2:
                    require(handles.erase(h) == 1, "handle freed once");
                    std::free(*h);
                    delete h;
                    break;
                case 3:
                    require(handles.count(h), "resize live handle");
                    *h = static_cast<char*>(std::realloc(*h, eb->fParam2));
                    require(*h, "handle resize allocation");
                    handles[h] = eb->fParam2;
                    break;
                case 4:
                    require(handles.count(h), "size live handle");
                    eb->fParam2 = handles[h];
                    break;
                case 5:
                    require(handles.count(h), "lock live handle");
                    eb->fParam3 = reinterpret_cast<sLONG_PTR>(*h);
                    break;
                case 6:
                    require(handles.count(h), "unlock live handle");
                    break;
                default:
                    require(false, "unmodeled handle operation");
            }
            eb->fError = 0;
            return;
        }
        case EX_GET_METHOD_ID:
            require(CUTF16String(eb->fUName) == expectedMethod, "method name preserved");
            eb->fParam1 = directMethod ? 42 : 0;
            eb->fError = 0;
            return;
        case EX_CALL_BY_PROCID: {
            require(eb->fParam1 == 42 && eb->fParam2 == 2, "direct callback signature");
            PA_Variable* values = reinterpret_cast<PA_Variable*>(*eb->fHandle);
            callback(&values[1], &values[2]);
            values[0] = callbackResult(values[2].uValue.fString);
            eb->fError = callbackError ? eER_IncompatibleTypes : 0;
            return;
        }
        case EX_EXECUTE_COMMAND_BY_ID: {
            PA_Variable** values = static_cast<PA_Variable**>(eb->fPtr1);
            if (eb->fParam1 == 1007) {
                require(eb->fParam2 == 3, "fallback callback signature");
                checkString(values[0]->uValue.fString, expectedMethod);
                callback(values[1], values[2]);
                // EXECUTE METHOD is a procedure; only the direct method API returns a value.
            } else if (eb->fParam1 == 678) {
                checkString(values[0]->uValue.fString, expectedContext);
                clear(values[1]);
                values[1]->fType = eVK_Picture;
                values[1]->uValue.fPicture = newPicture();
            } else if (eb->fParam1 == 1002) {
                checkString(values[1]->uValue.fString, expectedFormat);
                PA_Picture p = values[0]->uValue.fPicture;
                require(pictures.count(p), "convert owns picture");
            } else {
                require(false, "unmodeled command");
            }
            eb->fError = callbackError ? eER_IncompatibleTypes : 0;
            return;
        }
        case EX_CREATE_PICTURE:
            eb->fPicture = newPicture();
            eb->fError = 0;
            return;
        case EX_DISPOSE_PICTURE:
            releasePicture(eb->fPicture);
            eb->fError = 0;
            return;
        case EX_DUPLICATE_PICTURE: {
            PA_Picture p = eb->fPicture;
            require(pictures.count(p), "retain live picture");
            require(eb->fParam1 == 0, "retain-only picture contract");
            ++pictures[p];
            return;
        }
        case EX_GET_PICTURE_DATA:
            // Empty synthetic pictures have no representations.
            eb->fUniString1 = {};
            eb->fError = eER_IncompatibleTypes;
            return;
        case EX_CONVERT_CHARSET_TO_CHARSET: {
            require(eb->fParam2 == eVTC_UTF_8 && eb->fParam4 == eVTC_UTF_16,
                    "conversion encodings");
            require(eb->fParam3 == (eb->fParam1 + 1) * sizeof(PA_Unichar),
                    "destination capacity is bytes including terminator space");
            CFStringRef s = CFStringCreateWithBytes(kCFAllocatorDefault,
                static_cast<const UInt8*>(eb->fPtr1), eb->fParam1,
                kCFStringEncodingUTF8, false);
            require(s, "valid UTF-8 conversion fixture");
            CFIndex units = CFStringGetLength(s);
            require((units + 1) * sizeof(PA_Unichar) <= eb->fParam3, "conversion fits");
            CFStringGetCharacters(s, CFRangeMake(0, units),
                                  static_cast<UniChar*>(eb->fPtr2));
            static_cast<PA_Unichar*>(eb->fPtr2)[units] = 0;
            eb->fParam3 = units * sizeof(PA_Unichar);
            eb->fError = 0;
            CFRelease(s);
            return;
        }
        default:
            std::fprintf(stderr, "unmodeled engine selector %d\n", selector);
            std::abort();
    }
}

void balanced()
{
    require(strings.empty(), "all SDK string allocations released");
    require(handles.empty(), "all SDK handles released");
    require(pictures.empty(), "all SDK picture references released");
}

const PA_Unichar ascii[] = {'A', 'B', 'C', 0};
const PA_Unichar bmp[] = {0x03a9, 0x65e5, 0};
const PA_Unichar supplementary[] = {'X', 0xd83d, 0xde00, 'Y', 0};
const PA_Unichar empty[] = {0};
const PA_Unichar* fixtures[] = {ascii, bmp, supplementary, empty};

void testCallbacks()
{
    expectedMethod = CUTF16String(ascii);
    MK::WATCH_METHOD.setUTF16String(ascii, expectedMethod.size());
    for (bool allocated : {false, true}) {
        allocatedEmpty = allocated;
        for (bool direct : {false, true}) {
            directMethod = direct;
            returnKind = 0;
            for (const PA_Unichar* fixture : fixtures) {
                expectedContext = CUTF16String(fixture);
                MK::WATCH_CONTEXT.setUTF16String(fixture, expectedContext.size());
                for (int repeat = 0; repeat < 10; ++repeat) {
                    MK::MEDIA_KEY_CODES.push_back(NX_KEYTYPE_PLAY);
                    listenerLoopExecuteMethod();
                    require(MK::MEDIA_KEY_CODES.empty(), "queued key consumed");
                    balanced();
                }
            }
        }
    }
    require(calls == 160, "all callback argument combinations exercised");
}

void testCallbackResults()
{
    directMethod = true;
    expectedMethod = CUTF16String(ascii);
    MK::WATCH_METHOD.setUTF16String(ascii, expectedMethod.size());
    for (bool allocated : {false, true}) {
        allocatedEmpty = allocated;
        for (int kind : {1, 2}) {
            returnKind = kind;
            for (const PA_Unichar* fixture : fixtures) {
                expectedContext = CUTF16String(fixture);
                MK::WATCH_CONTEXT.setUTF16String(fixture, expectedContext.size());
                for (int repeat = 0; repeat < 10; ++repeat) {
                    MK::MEDIA_KEY_CODES.push_back(NX_KEYTYPE_PLAY);
                    listenerLoopExecuteMethod();
                    balanced();
                }
            }
        }
    }
    require(calls == 160, "all independent and copied-input results exercised");
}

void testCallbackErrors()
{
    expectedMethod = CUTF16String(ascii);
    expectedContext = CUTF16String(supplementary);
    MK::WATCH_METHOD.setUTF16String(ascii, expectedMethod.size());
    MK::WATCH_CONTEXT.setUTF16String(supplementary, expectedContext.size());
    for (bool allocated : {false, true}) {
        allocatedEmpty = allocated;
        for (bool direct : {false, true}) {
            directMethod = direct;
            failNextHandle = true;
            MK::MEDIA_KEY_CODES.push_back(NX_KEYTYPE_PLAY);
            unsigned before = calls;
            listenerLoopExecuteMethod();
            require(!failNextHandle && calls == before, "failed allocation never calls host method");
            balanced();
            callbackError = true;
            returnKind = 2;
            MK::MEDIA_KEY_CODES.push_back(NX_KEYTYPE_PLAY);
            listenerLoopExecuteMethod();
            require(calls == before + 1, "host error path called method once");
            balanced();
            callbackError = false;
        }
    }
}

void testPath()
{
    for (bool allocated : {false, true}) {
        allocatedEmpty = allocated;
        for (const PA_Unichar* fixture : fixtures) {
            expectedContext = CUTF16String(fixture);
            C_TEXT path;
            path.setUTF16String(fixture, expectedContext.size());
            CUTF8String utf8;
            path.copyUTF8String(&utf8);
            {
                CPicture picture;
                for (int repeat = 0; repeat < 10; ++repeat)
                    picture.fromFileAtPath(&utf8);
            }
            balanced();
        }
    }
}

void testConversion()
{
    const char* utf8[] = {"ABC", "\xce\xa9\xe6\x97\xa5", "X\xf0\x9f\x98\x80Y", ""};
    for (bool allocated : {false, true}) {
        allocatedEmpty = allocated;
        for (size_t i = 0; i < 4; ++i) {
            expectedFormat = CUTF16String(fixtures[i]);
            {
                CPicture picture;
                for (int repeat = 0; repeat < 10; ++repeat) {
                    PA_Picture copy = picture.createCopy(utf8[i]);
                    require(copy == picture.getPicture(), "retain-only copy preserved");
                    require(pictures[copy] == 2, "returned picture has independent owning reference");
                    PA_DisposePicture(copy);
                }
            }
            balanced();
        }
    }
}

void testArrays()
{
    for (bool allocated : {false, true}) {
        allocatedEmpty = allocated;
        PA_Variable array = PA_CreateVariable(eVK_ArrayUnicode);
        PA_ResizeArray(&array, 4);
        for (PA_long32 i = 0; i < 4; ++i) {
            PA_Unistring text = PA_CreateUnistring(const_cast<PA_Unichar*>(fixtures[i]));
            PA_SetStringInArray(array, i, &text);
        }
        ARRAY_TEXT copy;
        PA_Variable* parameter = &array;
        PackagePtr parameters = reinterpret_cast<PackagePtr>(&parameter);
        copy.fromParamAtIndex(parameters, 1);
        for (PA_long32 i = 0; i < 4; ++i) {
            PA_Unistring borrowed = PA_GetStringInArray(array, i);
            checkString(borrowed, CUTF16String(fixtures[i]));
        }
        // The helper copies borrowed content before mutation, then transfers distinct allocations.
        PA_Unistring changed = PA_CreateUnistring(const_cast<PA_Unichar*>(bmp));
        PA_SetStringInArray(array, 0, &changed);
        copy.toParamAtIndex(parameters, 1);
        for (PA_long32 i = 0; i < 4; ++i)
            checkString(PA_GetStringInArray(array, i), CUTF16String(fixtures[i]));
        PA_Unistring first = PA_GetStringInArray(array, 0);
        PA_Unistring independent = PA_CreateUnistring(first.fString);
        PA_SetStringInArray(array, 4, &independent);
        require(first.fString != PA_GetStringInArray(array, 4).fString, "independent element owners");
        PA_Unistring replacement = PA_CreateUnistring(const_cast<PA_Unichar*>(bmp));
        PA_SetStringInArray(array, 0, &replacement);
        checkString(PA_GetStringInArray(array, 4), CUTF16String(ascii));
        for (PA_long32 i : {-1, 5}) {
            PA_Unistring rejected = PA_CreateUnistring(const_cast<PA_Unichar*>(ascii));
            PA_SetStringInArray(array, i, &rejected);
            checkString(rejected, CUTF16String(ascii));
            PA_DisposeUnistring(&rejected);
        }
        PA_Variable wrong = PA_CreateVariable(eVK_Longint);
        PA_Variable noData = {};
        noData.fType = eVK_ArrayUnicode;
        for (PA_Variable destination : {wrong, noData}) {
            PA_Unistring rejected = PA_CreateUnistring(const_cast<PA_Unichar*>(bmp));
            PA_SetStringInArray(destination, 0, &rejected);
            checkString(rejected, CUTF16String(bmp));
            PA_DisposeUnistring(&rejected);
        }
        PA_SetStringInArray(array, 0, nullptr);
        checkString(PA_GetStringInArray(array, 0), CUTF16String(bmp));
        PA_ResizeArray(&array, 1);
        PA_ClearVariable(&array);
        balanced();
    }
}
}

int main(int argc, char** argv)
{
    @autoreleasepool {
        require(argc == 2, "one test case required");
        gCall4D = engine;
        if (!std::strcmp(argv[1], "callbacks")) testCallbacks();
        else if (!std::strcmp(argv[1], "callback-results")) testCallbackResults();
        else if (!std::strcmp(argv[1], "callback-errors")) testCallbackErrors();
        else if (!std::strcmp(argv[1], "path")) testPath();
        else if (!std::strcmp(argv[1], "conversion")) testConversion();
        else if (!std::strcmp(argv[1], "arrays")) testArrays();
        else require(false, "unknown test case");
        balanced();
        std::printf("PASS %s\n", argv[1]);
    }
}
