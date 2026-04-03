#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <filesystem>

#ifdef _WIN32
    #include <Windows.h>
    #include <stdexcept>
    #include <WinBase.h>
    #include <consoleapi.h>
    #pragma comment(lib, "user32.lib")
#endif

#ifdef __ANDROID__
    #include <android/log.h>
#endif



//Log defs
#ifndef P_LOG_SUCCESS

#define P_LOG_SUCCESS(message)  { Plum::P_LogOut::mainLogStream << Plum::P_LogOut::GREEN << message; Plum::P_LogOut::PrintCurrentStream(__FILE__, __LINE__); }

#define P_LOG_ERROR(message)    { Plum::P_LogOut::mainLogStream << Plum::P_LogOut::RED << message; Plum::P_LogOut::PrintCurrentStream(__FILE__, __LINE__); }

#define P_LOG_INFO(message)     { Plum::P_LogOut::mainLogStream << Plum::P_LogOut::CYAN << message; Plum::P_LogOut::PrintCurrentStream(__FILE__, __LINE__); }

#define P_LOG_DEFAULT(message)  { Plum::P_LogOut::mainLogStream << Plum::P_LogOut::RESET << message; Plum::P_LogOut::PrintCurrentStream(__FILE__, __LINE__); }

#define P_LOG_SPACE()           { Plum::P_LogOut::mainLogStream << Plum::P_LogOut::MAGENTA << "------" << __FUNCTION__ << "------"; Plum::P_LogOut::PrintCurrentStream(__FILE__, __LINE__); }

#define P_LOG_ARR(arr, amt)     { Plum::P_LogOut::LogArray(arr, amt);}
#define P_LOG_VEC(vec)          { Plum::P_LogOut::LogArray(vec.data(), vec.size()); }

#define P_LOG_IT() Plum::P_LogOut::LogIterative()
#define P_LOG_IT_RESET() Plum::P_LogOut::ResetIteration()

#endif


//Implementation
namespace Plum{

namespace P_LogOut{
//Cross-plattform console output
#pragma region DEFAULT

    inline std::stringstream mainLogStream = {};
    inline int exCounter = 0;
    
    //https://jakob-bagterp.github.io/colorist-for-python/ansi-escape-codes/standard-16-colors/
    inline const std::string RED = "\x1B[31m";
    inline const std::string GREEN = "\x1B[32m";
    inline const std::string YELLOW = "\x1B[33m";
    inline const std::string BLUE = "\x1B[34m";
    inline const std::string MAGENTA = "\x1B[35m";
    inline const std::string CYAN = "\x1B[36m";
    inline const std::string RESET = "\x1B[0m"; 


    inline void PrintCurrentStream(const char* file = "", int line = 0){
        using namespace std;
#ifdef DEBUG 

//Enable virtual terminal processing for colors
#ifdef _WIN32      
        //https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences 
        static bool colorChecked = false;

        while(!colorChecked){
            colorChecked = true;

            HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if(stdOut == INVALID_HANDLE_VALUE){
                std::cout << "Invalid handle for P_Logout! \n";
                break;
            }
            
            DWORD currentMode = 0;
            if(!GetConsoleMode(stdOut, &currentMode)){
                std::cout << "Cannot read consolemode for P_Logout! \n";
                break;
            }
            
            if(currentMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING != 0){
                std::cout << "Console-Mode already enabled!! \n";
                break;
            }

            if(!SetConsoleMode(stdOut, currentMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)){
                std::cout << "Cannot set consolemode for P_Logout! \n";
                break;
            }

            break;              
        }
#endif
  
        //Append message
        mainLogStream << " - (" << filesystem::path(file).filename() << ", " << line << ")";
        mainLogStream << RESET;
        mainLogStream << "\n";
        
        //Out
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "pengine.log", "%s", mainLogStream.str().c_str());
#else
        cout << mainLogStream.str() << std::endl;
#endif

        //Clear
        mainLogStream.str("");
        mainLogStream.clear();
#endif
    }

    inline void LogIterative(){
        mainLogStream << RESET << exCounter << " - iterative log";
        PrintCurrentStream();
        exCounter++;
    }

    inline void ResetIteration(){
        exCounter = 0;
    }

    template <typename T>
    inline void LogArray(T* data, int amount, int stride = 4){
        using namespace std;

        P_LOG_INFO("Logging Array from: " << data);
        for(int i = 0; i < amount; i++) {
            mainLogStream << RESET << data[i] << ", ";
            if( (i + 1) % stride == 0) PrintCurrentStream("Array-Size: ", amount);
        }
        if(amount % stride != 0) PrintCurrentStream();
        P_LOG_INFO("--- Logging Ended");
    }

#pragma endregion

}
}
