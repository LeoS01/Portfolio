### Description
Context: Custom game-engine.

This code represents the main entry point of my game-engine. Overall it works by providing a P_GameApp-class with unimplemented functions.
These functions are called in the OS-Specific entry points and can be defined per project for game-initialization, running and closing.

Windows:
Here it is achieved via the WinMain entry point~ where 'P_GameApp's functions are being called. Some coupling was required to make touch-input work on windows.

Android:
Gameworld/Graphics initalizations are seperated, as android may call these at arbitrary points (e.G. when tabbing out, graphics resources are released).
The main communication between the Android-OS and the game reside in PMainActivity.java~ where a JNI-Bridge talks to the game and fetches OS-Data (e.G. Input)


### Usage
#include <P_GameApp.h> and define each Plum::App::P_GameApp::Function in a project specific main.cpp.


### Known issues
Having each point in a seperate function can feel bloated, especially for smaller projects. A potential solution (not implemented yet due to low priority and 'backward-compatibility' concerns for current projects) would be to have a single 'point' (entry and exit, where the steps (OnCreated, OnDeleted, etc.) are passed via argument) and a single 'run' function.