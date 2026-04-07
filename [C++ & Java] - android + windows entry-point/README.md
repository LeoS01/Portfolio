### Description
Context: Custom game-engine.

This code represents the main entry point of my game-engine. Overall it works by providing a P_GameApp-class with unimplemented functions.
These functions are called in the OS-Specific entry points and can be defined per project for game-initialization, running and closing.

Windows:
Using the WinMain entry point~ calling'P_GameApp's functions. Some coupling with the input-module was required to make touch-input work on windows.

Android:
Gameworld/Graphics initalizations are seperated, as android may call these at arbitrary points (e.G. when tabbing out, graphics resources are released).
The main communication between the Android-OS and the game reside in PMainActivity.java~ where a JNI-Bridge talks to the game and fetches OS-Data (e.G. Input)


### Usage
#include <P_GameApp.h> and define each Plum::App::P_GameApp::Function in a project specific main.cpp.


### Known issues
- Seperation of OnResize and Handlemessage are mostly for different parameters~ unification could aid the developer-experience.
- The public enum 'Message' implies shared access to P_GameApp's messages. However, this iteration is written to be included in a single translation unit (otherwise yielding linkage errors)- potentially encouraging these errors, rendering it redundant.
- Despite being named 'game-app', basic features like delta-time or fixed-update computations aren't implemented in this class. While this was a conscious design decision (decoupling these computations as individual engine-headers to improve modularity) a name like 'P_RealtimeApp' might've been more precise.

> These errors haven't been fixed yet due to their low priority in comparison to other project-needs.