//Configs
#define P_DISPLAYNAME "Current Project"

//Debug
//#define MEASUREFPS

//Content
#include <P_System.h>
#include <P_Files/MODULE.h>
#include <P_App/MODULE.h>
#include <P_Common/P_FPSAnalyze.h>
#include <P_App/P_GameApp.h>
#include <P_Graphics/P_Graphics.h>
#include <P_Graphics/P_Framebuffer.h>
#include <P_Graphics/P_Resolution.h>
#include <P_Common/P_TimeClock.h>
#include <P_Common/P_ThreadWait.h>
#include <P_Common/P_TimedFunc.h>

#include "Input.h"
#include "_Config.h"
#include "BaseAssets.h"
#include "Input.h"
#include "Gizmo.h"
#include "GameManager.h"

#include "Player/Player.h"
#include "Camera/FSQ.h"
#include "Camera/WorldView.h"
#include "Environment/Environment.h"
#include "Environment/Background.h"
#include "UI/UIPage.h"
#include "UI/Layout_HUD.h"


#define USE_NAMESPACES \
    using namespace std; \
    using namespace _Config; \
    using namespace Plum; \
    using namespace Plum::Graphics; \
    using namespace Plum::Common; \

inline static Plum::Common::P_TimeClock mainClock = {};
inline static Plum::Common::P_TimedFunc fixedUpdate = {_Config::_fixedFPS, 0};

inline static Plum::Graphics::P_Framebuffer* frameBuffer = nullptr;
inline static Plum::Graphics::P_Framebuffer::Config frameBufferConfig = {1, _Config::_filterFrameBuffer};
inline static std::pair<int, int> resolution = {0, 0};
inline static std::pair<int, int> scaledResolution = {0, 0};
inline static float lastRunDeltaTime = 0;


void Plum::App::P_GameApp::HandleMessage(Plum::App::P_GameApp::Message msg){
    USE_NAMESPACES
    P_LOG_SPACE();
    P_LOG_INFO("Message: " << (int)msg)

    //Base handling
    switch (msg)
    {
        //Creation
        case P_GameApp::Message::CreateWorld:
            //Dependencies HAVE to be created first ~ therefore the ORDER matters here!
            P_Interface::Register<Input>();         //Input as early as possible
            P_Interface::Register<BaseAssets>();    //Baseassets also before anything that renders graphics
            P_Interface::Register<GameManager>();

            P_Interface::Register<Background>();    //Draw background first
            P_Interface::Register<Environment>();
            P_Interface::Register<Player>();
            P_Interface::Register<WorldView>();
            
            P_Interface::Register<Gizmo>();
            P_Interface::Register<UIPage>();
            P_Interface::Register<FSQ>();           //Post should be last (although currently it doesntr really matter...)  
            
            //UI-Layouts
            UIPage::RegisterPage<Layout_HUD>();
            UIPage::RequestPage<Layout_HUD>();

            //Call
            P_Interface::Point( {0, P_Interface::PT_CREATE} );
            P_LOG_SUCCESS("Created Game-World!");
        break;

        case P_GameApp::Message::CreateGraphics:  
            //Alloc
            resolution = GetResolution();
            scaledResolution = P_Resolution::GetClosestResolution(resolution, _Config::referenceResolution);
            
            //Apply
            P_Graphics::Create(mainHandle);   
            P_Interface::Point( {resolution, P_Interface::PT_GRAPHICS_CREATE} );
            OnResize(resolution);
            P_LOG_SUCCESS("Created Graphics!");
        break;


        //Deletion
        case P_GameApp::Message::DeleteGraphics:
            P_Graphics::Delete();
            delete frameBuffer; frameBuffer = nullptr;

            P_Interface::Point( {0, P_Interface::PT_GRAPHICS_DELETE} );
            P_LOG_SUCCESS("Released Graphics!");
        break;

        case P_GameApp::Message::DeleteWorld:
            P_Interface::Point( {0, P_Interface::PT_DELETE} );
            P_LOG_SUCCESS("Released Game-World!");
        break;
    }
}

void Plum::App::P_GameApp::OnResize(std::pair<int, int> newRez){
    USE_NAMESPACES
    P_LOG_SPACE();

    //Get
    resolution = newRez;
    scaledResolution = P_Resolution::GetClosestResolution(newRez, _Config::referenceResolution);
    P_LOG_INFO("Got resolution: " << scaledResolution.first << " | " << scaledResolution.second);
    
    //Set
    P_Graphics::Resize(newRez);

    if(frameBuffer != nullptr){ delete frameBuffer; frameBuffer = nullptr; }
    frameBuffer = new P_Framebuffer(scaledResolution.first, scaledResolution.second, frameBufferConfig);
    
    P_Interface::Point( {scaledResolution, P_Interface::PT_RESIZE} );
    P_LOG_SUCCESS("Resized game!");
}	


void OnFixed(float deltaTime_sec){
    using namespace Plum;

    P_Interface::Line( {deltaTime_sec, P_Interface::LN_FIXED} );
    Environment::CollideFor(deltaTime_sec, Player::GetPlayerInfo());
}

void Plum::App::P_GameApp::OnFrame(){
    USE_NAMESPACES

#ifdef _WIN32
    if(Input::GetOnce(Input::Fullscreen)) {
        Specifications::SwitchResolutionState();
    }
#endif

    //Delta-Time computation
    mainClock.RestartMeasure();
    float deltaTime_sec = mainClock.GetDeltaTime_Seconds() + lastRunDeltaTime; //Full delta time is sleep time + run time

    P_FPSAnalyze::StartCapture();

#ifdef MEASUREFPS
    P_FPSAnalyze::Enable();
    #else
    P_FPSAnalyze::Disable();
#endif

    //GameManagement
    P_FPSAnalyze::Capture("GameManagement");

    switch (GameManager::GetState())
    {
        case GameManager::State::RUNNING:
            //Fixed Update
            fixedUpdate.Tick_Fixed(deltaTime_sec, OnFixed);
            P_FPSAnalyze::Capture("Fixed");
            
            //Frame Update
            P_Interface::Line( {deltaTime_sec, P_Interface::LN_FRAME} );
            P_FPSAnalyze::Capture("Run");
        break;

        case GameManager::State::FREECAM:
            GameManager::LineSys( {deltaTime_sec, P_Interface::LN_FRAME} );
            Input::LineSys( {deltaTime_sec, P_Interface::LN_FRAME} );
            WorldView::Run_Free(deltaTime_sec);
        break;
    }

    
    //Draw
    P_Graphics::Clear(_clearColor);
    frameBuffer->SetActive_WriteTo();
    P_FPSAnalyze::Capture("Graphics-Setup");

    P_Interface::Line( {deltaTime_sec, P_Interface::LN_DRAW} );
    P_FPSAnalyze::Capture("World-Draw");


    FSQ::PostProcess(frameBuffer, resolution, scaledResolution);
    P_FPSAnalyze::Capture("PostProcessing");
    
    P_Graphics::Submit();
    P_FPSAnalyze::Capture("Submit");

    //ClampFPSThreadSleep
#ifdef _WIN32 //Android is vsynced either way
    mainClock.RestartMeasure();
    lastRunDeltaTime = mainClock.GetDeltaTime_Seconds();
    P_ThreadWait::FromPassedTime(mainClock.GetDeltaTime_Milliseconds(), _minFrameTime_milliseconds);
#endif
    
    P_FPSAnalyze::Capture("Sleep");
    P_FPSAnalyze::EndCapture();
}