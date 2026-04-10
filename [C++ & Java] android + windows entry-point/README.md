### Description
This code represents the main entry point of my game-engine. It works by providing a P_GameApp-class with yet unimplemented functions.
These functions are called in the OS-Specific entry points and can be defined per project for game-initialization, running and closing.

Gameworld/Graphics initializations are separated, as Android may call these at arbitrary points (e.G. when tabbing out, graphics resources are released).
The main communication between Android and the game resides in PMainActivity.java~ where a JNI-Bridge talks to the game and sends required data (e.G. input).


### Usage
#include <P_GameApp.h> and define each Plum::App::P_GameApp::Function in a project specific main.cpp.


### Known issues
- Seperation of OnResize and Handlemessage is mostly done solely for different parameters~ unification could aid the developer-experience.
- The public enum 'Message' implies shared access to P_GameApp's messages. However, this iteration is written to be included in a single translation unit (otherwise yielding linking-errors)- this design potentially encouraging these, rendering it redundant.

- Despite being named 'game-app', basic features like delta-time or fixed-update computations aren't implemented in this class. While this was a conscious design decision (decoupling these computations as individual engine-headers to improve modularity), a name like 'P_RealtimeApp' might've been more precise.

> These errors haven't been fixed yet due to their low priority in comparison to other project-needs.