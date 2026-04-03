//Configs
#define P_DISPLAYNAME "Lizard Steals Egg - The Videogame"

//Debug
//#define MEASUREFPS

//Content
#include <P_Files/MODULE.h>
#include <P_App/MODULE.h>

#include <P_Common/P_FPSAnalyze.h>
#include <P_App/P_GameApp.h>

#include <P_System.h>
#include <P_Graphics/P_Graphics.h>
#include <P_Graphics/P_Framebuffer.h>
#include <P_Graphics/P_Resolution.h>
#include <P_Common/P_TimeClock.h>
#include <P_Common/P_ThreadWait.h>
#include <P_Common/P_TimedFunc.h>

#include "_Config.h"
#include "Input.h"
#include "C_Assets.h"
#include "Input.h"
#include "Gizmo.h"
#include "GameManager.h"
#include "WorldView.h"

#include "UI/UI.h"
#include "Player/Player.h"
#include "Camera/FSQ.h"
#include "Environment/Environment.h"
#include "Environment/Background.h"

//UI
#include "UI/Layout_HUD.h"
#include "UI/Layout_PAUSE.h"


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


void LeaveNote(){
    P_LOG_SPACE();

    //TODO: Worldview, Environment and Entity still need proper P_Line implementation
    //Also perhaps rename to P_System? o.0 Unsure
    //Also assuming you finish UI and refactoring~
    //Perhaps DO rewrite game logging?
        //- Log Successes, Failures etc. directly to console
        //Keep track of EVERYTHING and dump a 'stacktrace' on crashes
        //Here perhaps the P_Log object COULD come in quite handy...
        //You could then have it 'configurable' to be turned on/off in builds for easier debugging
    //Also regarding memory~ you should add a 'P_Memory' object that conveniently renders the games memory...

    //So basically i need additional main analysis tools:
    //-> P_Log (logs to console and keeps custom stack trace)
    //-> P_Memory (monitors memory and can be printed out as a piechart)
    //-> P_FPSAnalyze (just as currently available, but needs some common format like P_Memory to be visualized in:)
    //
        //A -> P_Statistics (draws data from Memory or FPSanalyze as graphics)
        //B -> I think there is already a P_ASCIIGraph class? Perhaps just look into it and its format...

    P_LOG_DEFAULT("TODO: \n" <<
        "Perhaps have some abstraction system like gameobject or similar later on? \n" <<
        "QOL: Better resolution scaling (unsure what i meant by this...) \n" <<
        "Basic UI" <<
        "Aight your nex main goal should be to have a vertical slice version~ pretty UI, pretty graphics fun (but basic) gameplay, .svg icon etc."
        << "Known bugs:\n "
        << "Memory leak on resize (windows at least)"
    );
}

void Plum::App::P_GameApp::HandleMessage(Plum::App::P_GameApp::Message msg){
    USE_NAMESPACES
    P_LOG_SPACE();
    P_LOG_INFO("Message: " << (int)msg)

    //Base handling
    switch (msg)
    {
        //Creation
        case P_GameApp::Message::CreateWorld:
            //Dependencies HAVE to be created first ~ the order here matters!

            //First the 'logic' requirements
            P_Interface::Register<Input>();         //Input as early as possible
            P_Interface::Register<C_Assets>();    //Assets also before anything that renders graphics
            P_Interface::Register<GameManager>();
            P_Interface::Register<WorldView>();     

            //Then the world elements
            P_Interface::Register<Background>();    //Draw background first
            P_Interface::Register<Environment>();
            P_Interface::Register<Player>();

            //Finally the 'camera' elements
            P_Interface::Register<UI::UIElement>();
            P_Interface::Register<FSQ>();           //Post should be last (although currently it doesntr really matter...)

            //Call
            P_Interface::Point( {0, P_Interface::PT_CREATE} );
            P_LOG_SUCCESS("Created Game-World!");
        break;

        case P_GameApp::Message::CreateGraphics:  
            //Base
            resolution = GetResolution();
            scaledResolution = P_Resolution::GetClosestResolution(resolution, _Config::referenceResolution);
            
            P_Graphics::Create(mainHandle);
        #ifdef _WIN32   
            P_Graphics::SetVSync(0);
        #endif

            P_Interface::Point( {resolution, P_Interface::PT_GRAPHICS_CREATE} );
            OnResize(resolution);
            UI::UIElement::RequestLayout(UI::Layout_HUD);

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
            
            #ifdef DEBUG
            LeaveNote();
            #endif

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
    
    P_Graphics::Submit();       //Note: If you enable VSync on windows, specifically Submit() will block (duh)
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