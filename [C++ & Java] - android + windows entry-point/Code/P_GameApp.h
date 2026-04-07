#pragma once
#include <stdlib.h>
#include <string>
#include <sstream>
#include <csignal>
#include "../P_LogOut.h"

#ifdef _WIN32
    #include <iostream>
    #include <Windows.h>
    #include <vector>
    #pragma comment (lib, "user32.lib")
    #pragma comment(linker, "-subsystem:WINDOWS")

    #include "_Input/P_Touch.h"
#endif

#ifdef __ANDROID__
    #include <jni.h>
    #include <android/native_window.h>
    #include <android/native_window_jni.h>
    #include <thread>
    #include <utility>
#endif

//CONFIG
#ifndef P_DISPLAYNAME
    #define P_DISPLAYNAME "Plum Project"
#endif

#ifndef P_APP_WIN_SAFESIZE
    #define P_APP_WIN_SAFESIZE true
#endif


//DECLARATION
namespace Plum::App {

class P_GameApp{
//Project Interface
public:
    //Executed in this order
    enum class Message : char{
        CreateWorld = 0,
        CreateGraphics = 1,
        DeleteGraphics = 2,
        DeleteWorld = 3
    };

    //v overwrite these functions!
    //Don't forget to add the 'Plum::App::P_GameApp::' prefix!^^
    static void HandleMessage(Message msg);
    static void OnFrame();
    static void OnResize(std::pair<int, int>);

    //Windows: HWND
    //Android: ANativeWindow*
    inline static void* mainHandle = nullptr;	


//Engine
public:
    struct Config{
    public:
        inline Config(std::string name){
            this->name = name;
        }
        std::string name = "P_GameApp";
    };

public:
    inline static void Create(P_GameApp::Config cfg);	
    inline static bool Run();
    inline static void Destroy();

private:
    struct Specifications;
    inline static Specifications* specs = nullptr;

public:
    inline static void* GetMainHandle(){ return mainHandle; }
    inline static void SetMainHandle(void* data){ mainHandle = data;}
    inline static std::pair<int, int> GetResolution();
    inline static float GetAspectRatio_wh(){
        std::pair<int, int> rez = GetResolution();
        return rez.first / static_cast<float>(rez.second);
    }

public:
    inline static void HandleSignal(int param){
        //https://www.geeksforgeeks.org/cpp/exit-vs-_exit-c-cpp/
        //https://stackoverflow.com/questions/33922223/what-exactly-sig-dfl-do

        //Handle
        switch (param)
        {
            case SIGABRT:
                P_LOG_ERROR("FATAL ERROR: got abnormal signal");
                break;

            case SIGILL:
                P_LOG_ERROR("FATAL ERROR: Illegal function call!");
                break;

            case SIGSEGV:
                P_LOG_ERROR("FATAL ERROR: Segfault!");
                break;
        }
        
        //Close
        P_LOG_ERROR("Terminating instance...");
        SIG_DFL(param);
        exit(param);
    }

    #ifdef _WIN32
    inline static LONG WINAPI SignalTranslation(LPEXCEPTION_POINTERS ptrs){
        //https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-setunhandledexceptionfilter
        std::cout << "\x1B[31m FATAL ERROR" << std::endl;   //(ANSI Red)
        return EXCEPTION_EXECUTE_HANDLER;
    }
    #endif

    inline static void InitSignalHandle(){
    #ifdef _WIN32
        SetUnhandledExceptionFilter(SignalTranslation);
    #else
        signal(SIGABRT, HandleSignal);
        signal(SIGILL, HandleSignal);
        signal(SIGSEGV, HandleSignal);
    #endif
    }
};

}



//WINDOWS_CLASSDEF
#ifdef _WIN32

namespace Plum::App {

struct P_GameApp::Specifications{
private:
    friend class P_GameApp;
    inline static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    inline static std::pair<int, int> GetClientRez(){
        std::pair<int, int> rezWH = {0, 0};
        RECT client = {};
        if(GetClientRect(window, &client)){
            rezWH.first = client.right - client.left;
            rezWH.second = client.bottom - client.top;
        } else {
            std::cout << __FILE__ << " - Error: Could not get windows client-rect \n";
        }
        return rezWH;
    }

    inline static void SetFullscreenBorderless(){
        using namespace std;
        //https://stackoverflow.com/questions/2382464/win32-full-screen-and-hiding-taskbar
        //https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-monitorfromwindow

        HMONITOR thisMon = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);	
        MONITORINFO mInfo = {};
        memset(&mInfo, 0, sizeof(MONITORINFO));
        mInfo.cbSize = sizeof(MONITORINFO);

        if(!GetMonitorInfo(thisMon, &mInfo)){
            P_LOG_ERROR("Failed to get monitor/screen info!");
            return;
        }


        RECT mRect = mInfo.rcMonitor;
        pair<int, int> screenRez = { mRect.right - mRect.left, mRect.bottom - mRect.top };

    #ifdef P_APP_WIN_SAFESIZE
        const int BUG_ADJUSTMENT = 1;		//For whatever reason, having the EXACT Window-Size seems to cause problems...
        screenRez.first += BUG_ADJUSTMENT;
        screenRez.second += BUG_ADJUSTMENT;
    #endif


        SetWindowLong(window, GWL_STYLE, borderlessFullscreenStyle);
        SetWindowPos(window, HWND_TOPMOST, mRect.left, mRect.top, screenRez.first, screenRez.second, SWP_FRAMECHANGED);	

        isFullScreen = true;
    }
    
    inline static void SetWindowed(std::pair<int, int> targetRez){
        using namespace std;
        
        pair<int, int> screenCenter = { GetSystemMetrics(SM_CXSCREEN) / 2, GetSystemMetrics(SM_CYSCREEN) / 2};
        pair<int, int> topLeftAsCenter = {screenCenter.first - targetRez.first / 2, screenCenter.second - targetRez.second / 2};
        
        SetWindowLong(window, GWL_STYLE, windowedStyle);
        SetWindowPos(window, HWND_TOP, 
            topLeftAsCenter.first, topLeftAsCenter.second, 
            targetRez.first, targetRez.second, 
            SWP_FRAMECHANGED);

        isFullScreen = false;	
    }

    inline static void SwitchResolutionState(){
        using namespace std;
        isFullScreen = !isFullScreen;

        if(isFullScreen){
            SetFullscreenBorderless();
        } else{
            pair<int, int> screenRez = { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
            pair<int, int> smallWindowResolution = {screenRez.first / 2, screenRez.second / 2};
            SetWindowed(smallWindowResolution);
        }
    }

    inline ~Specifications(){
        DestroyWindow(window);
    }
    
private:
    //https://learn.microsoft.com/en-us/windows/win32/winmsg/window-styles
    inline static long borderlessFullscreenStyle =  WS_VISIBLE;
    inline static long windowedStyle = WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
    inline static bool isFullScreen = false;
    inline static bool isAlive = true;

    inline static HWND window = nullptr;
};



inline void P_GameApp::Create(P_GameApp::Config cfg){
    using namespace std;
    
    SetProcessDPIAware();
    specs = new P_GameApp::Specifications();

    //Window-Name to correct format (WCHAR), https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
    int nameLength = MultiByteToWideChar(CP_UTF8, 0, cfg.name.c_str(), -1, nullptr, 0);
    vector<WCHAR> windowName;
    windowName.resize(nameLength);
    MultiByteToWideChar(CP_UTF8, 0, cfg.name.c_str(), -1, windowName.data(), windowName.size());

    //Register WNDCLASS, https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerclassa
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(nullptr);
    WNDCLASS winClass = {};
    winClass.lpfnWndProc = specs->WindowProc;
    winClass.hInstance = hInstance;
    const int _resourceIDOfIcon = 1;
    winClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE( _resourceIDOfIcon ));
    winClass.lpszClassName = cfg.name.c_str();
    RegisterClass(&winClass);

    //Create Window, https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowa
    pair<int, int> screenRez = { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    const float _rezScale = .5f;
    specs->window = CreateWindow(cfg.name.c_str(), cfg.name.c_str(), specs->windowedStyle, 
        screenRez.first * _rezScale * .5f, screenRez.second * _rezScale * .5f, 
        screenRez.first * _rezScale, screenRez.second * _rezScale, 
        NULL, NULL, hInstance, NULL);
        
    if(specs->window == nullptr || !specs->isAlive){
        P_LOG_ERROR("Error: Failed to register window!");
        return;
    }
    mainHandle = specs->window;
    
    //Register touch
    //https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registertouchwindow
    //https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getsystemmetrics
    P_Touch::Bridge::isTouchable = GetSystemMetrics(SM_DIGITIZER) & NID_EXTERNAL_PEN != 0;  //only count as being touchable when more than a pen is available
    RegisterTouchWindow(specs->window, 0);      //register no matter what. Returns 0 on failure
}


inline bool P_GameApp::Run(){
    //https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-peekmessagea
    //https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-translatemessage
    //https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-dispatchmessage
    MSG msg = {};
    while(PeekMessage(&msg, specs->window, NULL, NULL, PM_REMOVE)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return specs->isAlive;
}

inline void P_GameApp::Destroy(){
    delete specs; 
    UnregisterTouchWindow(specs->window);
}

inline std::pair<int, int> P_GameApp::GetResolution(){
    using namespace std;

    pair<int, int> rezWH = {0, 0};
    RECT client = {};
    if(GetClientRect(specs->window, &client)){
        rezWH.first = client.right - client.left;
        rezWH.second = client.bottom - client.top;
    } else {
        P_LOG_ERROR("Error: Could not get windows client-rect");
    }
    return rezWH;
}

}

#endif



//WINDOWS_CONNECTION
#ifdef _WIN32

inline void Win_OpenFatalErrorWindow(const char* msg){
    MessageBoxA(NULL, msg, "Oops! >.<", MB_ICONERROR | MB_OK);
}

inline LRESULT CALLBACK Plum::App::P_GameApp::Specifications::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam){ 
    //https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-defwindowproca
    using namespace Plum::App;
    using namespace std;

    //Validate
    if(!isAlive) return DefWindowProc(hwnd, msg, wparam, lparam);

    //Main Switch
    try
    {
        switch (msg)
        {
            //Create
            //https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-create
            case WM_CREATE:
                window = hwnd;
                mainHandle = window;
                P_GameApp::HandleMessage(P_GameApp::Message::CreateWorld);
                P_GameApp::HandleMessage(P_GameApp::Message::CreateGraphics);
                break;

            //Resize
            //https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-size
            case WM_SIZE:
                P_GameApp::OnResize( {static_cast<int>(LOWORD(lparam)), static_cast<int>(HIWORD(lparam))} );
                break;

            //Delete
            //https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-close
            case WM_CLOSE:
                isAlive = false;
                P_GameApp::HandleMessage(P_GameApp::Message::DeleteGraphics);
                P_GameApp::HandleMessage(P_GameApp::Message::DeleteWorld);
                return 0;

            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;


            //Touch Update
            //https://learn.microsoft.com/en-us/windows/win32/learnwin32/writing-the-window-procedure
            //https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getpointerinfo
            //https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-get_pointerid_wparam
            case WM_POINTERDOWN:
            case WM_POINTERUPDATE:
            case WM_POINTERUP:
                UINT32 pointerID = GET_POINTERID_WPARAM(wparam);
                POINTER_INFO info = {};

                //Checks
                if(!GetPointerInfo(pointerID, &info)){
                    P_LOG_ERROR("Couldn't get touch-pointer info!");
                    break;
                }
                if(info.pointerType != PT_TOUCH) break;


                switch(msg){
                    case WM_POINTERDOWN:
                        P_Touch::Specifications::AddTouchInput((int)pointerID, {info.ptPixelLocation.x, info.ptPixelLocation.y}, hwnd);

                    case WM_POINTERUPDATE:
                        P_Touch::Specifications::SetTouchInput((int)pointerID, {info.ptPixelLocation.x, info.ptPixelLocation.y}, hwnd);
                        break;

                    case WM_POINTERUP:
                        P_Touch::Specifications::RemoveTouchInput((int)pointerID);
                        break;
                }

                break;

        }
    }
    catch(const std::exception& e)
    {
        Win_OpenFatalErrorWindow(e.what());
        isAlive = false;
        PostQuitMessage(-1);
        return 0;
    }
    
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

//https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int){
    using namespace Plum::App;

    //Init
    //Lifecycle Handling in mainproc
    P_GameApp::InitSignalHandle();
    try
    {
        //Init
        P_GameApp::Config cfg(P_DISPLAYNAME);
        P_GameApp::Create(cfg);   

        //Run
        while (P_GameApp::Run()){
            P_GameApp::OnFrame();
        }

        P_GameApp::Destroy();
        P_LOG_SUCCESS("Application is returning!");
        return 0;
    }
    catch(const std::exception& e)
    {
        P_LOG_ERROR(e.what());
        Win_OpenFatalErrorWindow(e.what());
        P_LOG_SUCCESS("Application is returning!");
        return -1;
    }

    P_LOG_SUCCESS("Application is returning!");
    return 0;
}

#endif



//ANDROID_CLASSDEF
#ifdef __ANDROID__

namespace Plum::App {

struct P_GameApp::Specifications{
public:
    Specifications(){
        isAppAlive = true;
    }

    ~Specifications(){
        isAppAlive = false;
    }

    bool isAppAlive = {};
};


inline void P_GameApp::Create(P_GameApp::Config cfg){
    specs = new Specifications();
}

inline bool P_GameApp::Run(){
    if(specs == nullptr) return false;
    return specs->isAppAlive;
}

inline void P_GameApp::Destroy(){
    specs->isAppAlive = false;
    
    delete specs;
    specs = nullptr;
}

inline std::pair<int, int> P_GameApp::GetResolution(){
    ANativeWindow* window = reinterpret_cast<ANativeWindow*>( mainHandle );
    return { ANativeWindow_getWidth(window), ANativeWindow_getHeight(window) };
}

}
#endif



//ANDROID_CONNECTION
#ifdef __ANDROID__

#ifndef P_AndroidVoid_App
	#define P_AndroidVoid_App(name) extern "C" JNIEXPORT void JNICALL Java_P_1App__1Android_PMainActivity_##name
#endif

inline void Android_ThrowError(JNIEnv* env, const char* msg){
    P_LOG_ERROR(msg);
    env->ThrowNew( env->FindClass("java/lang/RuntimeException"), msg);
}


//Entry points
P_AndroidVoid_App(GameAppDummy)(JNIEnv* env, jobject){
    P_LOG_SUCCESS("Loaded Game-App Module for Android!");
}

P_AndroidVoid_App(InitGameApp)(JNIEnv* env, jobject){
    using namespace Plum::App;
    P_GameApp::InitSignalHandle();

    try{
        P_GameApp::Config cfg(P_DISPLAYNAME);
        P_GameApp::Create(cfg);  

        P_GameApp::HandleMessage(P_GameApp::Message::CreateWorld);
    } catch (const std::exception& e){    
        Android_ThrowError(env, e.what());
    }
}


P_AndroidVoid_App(InitAppGraphics)(JNIEnv* env, jobject, jobject surface){
    using namespace Plum::App;
    try{
        P_GameApp::SetMainHandle( ANativeWindow_fromSurface(env, surface) );
        P_LOG_SUCCESS("Aquired ANAtiveWindow!");

        P_GameApp::HandleMessage(P_GameApp::Message::CreateGraphics);
    } catch (const std::exception& e){    
        Android_ThrowError(env, e.what());
    }
}

P_AndroidVoid_App(RunGameApp)(JNIEnv* env, jobject){
    using namespace Plum::App;
    try{
        P_GameApp::OnFrame();
    } catch (const std::exception& e){    
        Android_ThrowError(env, e.what());
    }
}

P_AndroidVoid_App(ResizeAppGraphics)(JNIEnv* env, jobject, int w, int h){
    using namespace Plum::App;
    try{
        P_GameApp::OnResize( {w, h} );
    } catch (const std::exception& e){    
        Android_ThrowError(env, e.what());
    }
}

P_AndroidVoid_App(CloseAppGraphics)(JNIEnv* env, jobject){
    using namespace Plum::App;
    try{
        ANativeWindow_release(reinterpret_cast<ANativeWindow*>( P_GameApp::GetMainHandle() ));
        P_GameApp::SetMainHandle(nullptr);
        P_LOG_SUCCESS("Released ANativeWindow!");

       P_GameApp::HandleMessage(P_GameApp::Message::DeleteGraphics);
    } catch (const std::exception& e){    
        Android_ThrowError(env, e.what());
    }
}

P_AndroidVoid_App(CloseGameApp)(JNIEnv* env, jobject){
    using namespace Plum::App;

    try{
        P_GameApp::HandleMessage(P_GameApp::Message::DeleteWorld);
        P_GameApp::Destroy();
    } catch (const std::exception& e){    
        Android_ThrowError(env, e.what());
    }

    P_LOG_SUCCESS("Successfully closed the app!");
}

#endif
