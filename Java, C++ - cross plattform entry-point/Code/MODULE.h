#pragma once
//Include this ONCE per project in ONE .cpp file

#ifdef __ANDROID__
#include <jni.h>
#include <vector>
#include <utility>
#include <map>

#include "../P_LogOut.h"
#include "_Input/P_Touch.h"

#ifndef P_AndroidVoid_App
	#define P_AndroidVoid_App(name) extern "C" JNIEXPORT void JNICALL Java_P_1App__1Android_PMainActivity_##name
#endif



namespace Plum::App::Bridge{
//Utils
inline void Android_ThrowError(JNIEnv* env, const char* msg){
    env->ThrowNew( env->FindClass("java/lang/RuntimeException"), msg);
}
    
#pragma region SCREENINFO
    int android_dpi = 0;
    std::pair<int, int> android_screen_resolution_px = {0, 0};

    P_AndroidVoid_App( ScreenInfoDummy )(JNIEnv*, jobject){
        P_LOG_SUCCESS("Loaded Screen-Info Module for Android!");
    }

    P_AndroidVoid_App( SetScreenDPI )(JNIEnv*, jobject, int dpi){
        android_dpi = dpi;
    }   

    P_AndroidVoid_App( SetScreenResolution )(JNIEnv*, jobject, int w_px, int h_px){
        android_screen_resolution_px = {w_px, h_px};
    }
#pragma endregion



#pragma region TOUCH
    P_AndroidVoid_App(InputTouchDummy)(JNIEnv*, jobject){
        P_LOG_SUCCESS("Loaded Touch-Module for android!");
	}

    P_AndroidVoid_App(InputAddTouch)(JNIEnv* env, jobject, int id, float posX, float posY){
        using namespace Plum::App;

        P_Touch::Bridge::AddTouch(id, {posX, posY} );
	}
	
	P_AndroidVoid_App(InputSetTouch)(JNIEnv*, jobject, int id, float posX, float posY){
        using namespace Plum::App;

        P_Touch::Bridge::UpdateTouch(id, {posX, posY} );
	}
    
    P_AndroidVoid_App(InputRemoveTouch)(JNIEnv*, jobject, int id){
        using namespace Plum::App;

        P_Touch::Bridge::RemoveTouch(id);
	}
#pragma endregion



#pragma region CONTROLLER
    std::map<int, int> androidControllerIDToIndex = {};
    std::vector< std::map<int, float> > controllerStates = {};

    P_AndroidVoid_App(InputControllerDummy)(JNIEnv*, jobject){
        P_LOG_SUCCESS("Loaded Controller-Module for android!");
	}

    P_AndroidVoid_App(AddController)(JNIEnv* env, jobject, int controllerID){
        androidControllerIDToIndex[controllerID] = controllerStates.size();
        controllerStates.push_back({});

        P_LOG_SUCCESS("Added controller '" << controllerID << "' !");
    }
    
    P_AndroidVoid_App(RemoveController)(JNIEnv* env, jobject, int controllerID){
        //RemoveFromSets
        int initialIndex = androidControllerIDToIndex[controllerID];
        controllerStates.erase(controllerStates.begin() + initialIndex);
        androidControllerIDToIndex.erase(controllerID);

        //AdjustRemainder
        for(auto& key : androidControllerIDToIndex){
            if(key.second > initialIndex) key.second--;
        }

        P_LOG_SUCCESS("Removed controller '" << controllerID << "' !");
    }


    P_AndroidVoid_App(InputControllerAxisUpdate)(JNIEnv* env, jobject, int controllerID, int axisIndex, float axisValue){
        int indexInVector = androidControllerIDToIndex[controllerID];
        controllerStates[indexInVector][axisIndex] = axisValue;
    }

    P_AndroidVoid_App(InputControllerButtonDown)(JNIEnv*, jobject, int controllerID, int buttonIndex){
        int indexInVector = androidControllerIDToIndex[controllerID];
        controllerStates[indexInVector][buttonIndex] = 1;
    }
    
    P_AndroidVoid_App(InputControllerButtonUp)(JNIEnv*, jobject, int controllerID, int buttonIndex){
        int indexInVector = androidControllerIDToIndex[controllerID];
        controllerStates[indexInVector][buttonIndex] = 0;
    }
#pragma endregion



#pragma region KEYBOARD
    std::map<int, bool> androidKeyboardButtonStates;
    
    P_AndroidVoid_App(InputKeyboardDummy)(JNIEnv*, jobject){
        P_LOG_SUCCESS("Loaded keyboard module for android!");
    }

    P_AndroidVoid_App(InputKeyboardButtonDown)(JNIEnv*, jobject, int buttonID){
        androidKeyboardButtonStates[buttonID] = true;
    }
    
    P_AndroidVoid_App(InputKeyboardButtonUp)(JNIEnv*, jobject, int buttonID){
        androidKeyboardButtonStates[buttonID] = false;
    }
    
#pragma endregion

}

#endif