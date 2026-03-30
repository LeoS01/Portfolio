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
public:
    struct Config{
    public:
        Config(std::string name){
            this->name = name;
        }
        std::string name = "P_GameApp";
    };

public:
    inline static void Create(P_GameApp::Config cfg);	
    inline static bool Run();
    inline static void Destroy();
    
    
public:
    //User defined methods, executed in order of suffix*
    static void CreateGameWorld_0();    
    static void CreateGraphics_1();
    
    static void RunApp_2();
    static void OnResize_2(std::pair<int, int>);

    static void ReleaseGraphics_3();
    static void ReleaseGameWorld_4();

    //*Graphics aswell as gameworld may be created/released many times at arbitrary points on android
private:
    struct Specifications;
    inline static Specifications* specs = nullptr;

public:
    //Windows: HWND
    //Android: ANativeWindow*
    inline static void* mainHandle = nullptr;	

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

    inline static void InitSignalHandle(){
#ifdef DEBUG
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
        P_GameApp::OnResize_2( GetClientRez() );
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
        P_GameApp::OnResize_2( GetClientRez() );
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
    inline static long windowedStyle = WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU;
    inline static bool isFullScreen = false;
    inline static bool isAlive = true;

    inline static HWND window = nullptr;
};



inline void P_GameApp::Create(P_GameApp::Config cfg){
    using namespace std;
    
    SetProcessDPIAware();
    P_GameApp::InitSignalHandle();
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
        
    if(specs->window == nullptr){
        P_LOG_ERROR("Error: Failed to register window!");
        return;
    }
        
    RegisterTouchWindow(specs->window, 0);	//if no touch supported this will just return failure~
    mainHandle = specs->window;
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
    //https://learn.microsoft.com/en-us/windows/win32/learnwin32/writing-the-window-procedure
    //https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getpointerinfo
    //https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-get_pointerid_wparam

    using namespace Plum::App;
    using namespace std;

    //Checks
    if(msg == WM_DESTROY || msg == WM_CLOSE || msg == WM_QUIT){ isAlive = false; }

    //Main Switch
    switch (msg)
    {
        //Touch Update
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

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

//https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int){
    using namespace Plum::App;

    //Alloc
    std::string prefix = "Failed game during init: ";
    std::stringstream errorMsg = {};
    int errorCode = 0;
    
    //Init
    try
    {
        P_GameApp::Config cfg(P_DISPLAYNAME);
        P_GameApp::Create(cfg);   
        P_GameApp::CreateGameWorld_0();
        P_GameApp::CreateGraphics_1();
        //OnResize_2 is called in Windows-Definition! (SwitchResolutionState)

        prefix = "Failed game during main-loop: ";
        //Run
        while (P_GameApp::Run()){
            P_GameApp::RunApp_2();
        }
    }
    catch(const std::exception& e)
    {
        errorMsg << prefix << e.what();
        Win_OpenFatalErrorWindow(errorMsg.str().c_str());
        errorCode -= 1;
    }


    //Close
    try{
        P_GameApp::ReleaseGraphics_3();
        P_GameApp::ReleaseGameWorld_4();
        P_GameApp::Destroy();
    }
    catch(const std::exception& e)
    {
        errorMsg << "Failed closing the game: " << e.what();
        Win_OpenFatalErrorWindow(errorMsg.str().c_str());
        errorCode -= 1;
    }

    if(errorCode == 0) P_LOG_SUCCESS("Successfully closed app!");
    if(errorCode != 0) P_LOG_ERROR("Closed app with errors");
    return errorCode;
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

        P_GameApp::CreateGameWorld_0(); 
    } catch (const std::exception& e){    
        Android_ThrowError(env, e.what());
    }
}


P_AndroidVoid_App(InitAppGraphics)(JNIEnv* env, jobject, jobject surface){
    using namespace Plum::App;
    try{
        P_GameApp::SetMainHandle( ANativeWindow_fromSurface(env, surface) );
        P_LOG_SUCCESS("Aquired ANAtiveWindow!");

        P_GameApp::CreateGraphics_1();
    } catch (const std::exception& e){    
        Android_ThrowError(env, e.what());
    }
}

P_AndroidVoid_App(RunGameApp)(JNIEnv* env, jobject){
    using namespace Plum::App;
    try{
        P_GameApp::RunApp_2();
    } catch (const std::exception& e){    
        Android_ThrowError(env, e.what());
    }
}

P_AndroidVoid_App(ResizeAppGraphics)(JNIEnv* env, jobject, int w, int h){
    using namespace Plum::App;
    try{
        P_GameApp::OnResize_2( {w, h} );
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

        P_GameApp::ReleaseGraphics_3();
    } catch (const std::exception& e){    
        Android_ThrowError(env, e.what());
    }
}

P_AndroidVoid_App(CloseGameApp)(JNIEnv* env, jobject){
    using namespace Plum::App;

    try{
        P_GameApp::ReleaseGameWorld_4();
        P_GameApp::Destroy();
    } catch (const std::exception& e){    
        Android_ThrowError(env, e.what());
    }

    P_LOG_SUCCESS("Successfully closed the app!");
}

#endif
