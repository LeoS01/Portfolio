#pragma once
#include <initializer_list>
#include "P_LogOut.h"



namespace Plum{
    struct P_UnitTest{
    public:
        inline P_UnitTest(std::initializer_list< bool(*)() > tests, const char* msg = nullptr){
            //Init
            P_LOG_SPACE();
            if(msg != nullptr) P_LOG_INFO(msg);
            int passed = 0;

            //Run
            for(int i = 0; i < tests.size(); i++){
                try{
                    //Validate
                    if(tests.begin()[i] == nullptr){
                        P_LOG_ERROR("Function at index: " << i << "was nullptr!");
                        continue;
                    }

                    //Do
                    tests.begin()[i]();
                    passed++;
                } catch(void* e) {
                    P_LOG_ERROR("Failed to execute function at: " << i);   
                }         
            }

            //Evaluate
            if(passed == tests.size()){
                P_LOG_SUCCESS("Passed " << passed << "/" << tests.size() << " tests");
            } else {
                P_LOG_ERROR("Failed " << tests.size() - passed << " tests from " << tests.size());
            }
        }
    
    public:
        inline static bool CheckValidity(bool testResult, void* input, int inputBufferLength, void* compareTo, int compareBufferLength, bool allowLog = true){
            //Utils
            unsigned char* inputBuffer = reinterpret_cast<unsigned char*>(input);
            unsigned char* compareBuffer = reinterpret_cast<unsigned char*>(compareTo);

            auto Error = [inputBuffer, inputBufferLength, compareBuffer, compareBufferLength, allowLog](const char* msg){
                if(allowLog){
                    P_LOG_ERROR(msg);
                    P_LOG_INFO("Passed args:");
                    P_LOG_INFO(inputBuffer << " = input buffer");
                    P_LOG_INFO(inputBufferLength << " = input buffer length");
                    P_LOG_INFO(compareBuffer << " = compare buffer");
                    P_LOG_INFO(compareBufferLength << " = compare buffer length");
                }
                return false;
            };

            auto Fail = [allowLog](const char* msg){
                if(allowLog) P_LOG_ERROR(msg);
                return false;
            };
            

            //Validate
            if(inputBuffer == nullptr || compareBuffer == nullptr) return Error("Invalid buffer!");
            if(inputBufferLength <= 0 || compareBufferLength <= 0) return Error("Invalid buffer length!");
            

            //Run
			if(testResult){
                if(inputBufferLength != compareBufferLength) return Fail("Result-Length deviation!");

				//Validate Output
				if(memcmp(compareBuffer, inputBuffer, compareBufferLength) != 0){
                    if(allowLog){
                        P_LOG_ERROR("Unexpected data:");
                        P_LOG_ARR(compareBuffer, compareBufferLength);
                        
                        P_LOG_ERROR("Expected Data:");
                        P_LOG_ARR(compareBuffer, compareBufferLength);
                    }
                    return Fail("Deviantion in result values!");
				}

			} else{	
                return Fail("Failed valid-test!");
			}

            //Passed
            return true;
        }
    
        inline static bool EvaluateResult(bool finalResult, const char* testName){
            if(finalResult && testName != nullptr) P_LOG_SUCCESS("Passed test: " << testName);
            if(!finalResult && testName != nullptr) P_LOG_ERROR("Failed test: " << testName);
            return finalResult;
        }
    };


//Test example
#ifdef DEBUG
    namespace UnitTestTest::Validity{
        struct Args{
            bool result = false;

            unsigned char* dataA = nullptr;
            int dataALength = 0;

            unsigned char* dataB = nullptr;
            int dataBLength = 0;
        };

        inline bool TestValidityCheck(){
            //Alloc
            bool result = true;
            auto DoTest = [](Args& arg, bool log){
                return P_UnitTest::CheckValidity(arg.result, arg.dataA, arg.dataALength, arg.dataB, arg.dataBLength, log);
            };


            //Config valid test
            unsigned char dataA[] = {0x00, 0x01, 0x02};
            const int aLength_bytes = 3;

            Args validArg = {true, dataA, aLength_bytes, dataA, aLength_bytes};

            //Run valid-test
            if(!DoTest(validArg, true)){
                P_LOG_ERROR("P_UnitTest validity failed!");
                result = false;
            }


            //Config invalid test
            unsigned char dataB[] = {0x10, 0x11, 0x12, 0x13};
            const int bLength_bytes = 4;

            const int invalidArgCount = 5;
            Args invalidArgs[invalidArgCount] = {};
            for(int i = 0; i < invalidArgCount; i++) invalidArgs[i] = validArg;

            invalidArgs[0].result = false;
            invalidArgs[1].dataA = nullptr;
            invalidArgs[2].dataB = nullptr;
            invalidArgs[3].dataALength = -1;
            invalidArgs[4].dataBLength = -1;
            
            //Run invalid test
            for(int i = 0; i < invalidArgCount; i++){
                if(DoTest(invalidArgs[i], false)){
                    P_LOG_ERROR("P_UnitTest invalid failed as valid!");
                    result = false;
                }
            }

            return P_UnitTest::EvaluateResult(result, "P_UnitTest CheckValidity");
        }
    }

    namespace UnitTestTest{
        inline P_UnitTest unitTestTest = { {&Validity::TestValidityCheck} , __FILE__};
    }
#endif

}