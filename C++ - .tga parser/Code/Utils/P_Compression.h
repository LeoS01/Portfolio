#pragma once
#include <vector>
#include <limits>
#include "P_LogOut.h"
#include "P_UnitTest.h"

namespace Plum::Files{
    namespace P_Compression{
        inline const int _maxPacketSize = 256 * 1024;                //256kb
	    inline const int _maxDataSize = 10 * 1024 * 1024;            //10mb


        //Data format: 1 byte = packet-header (1st bit (MSB) = type, 2-8 = count); n-bytes: data; (repeat)
        inline bool RLE_Decompress(std::vector<unsigned char>& destination, unsigned char* rleData, int validDatalength_bytes, int expectedOutputLength_bytes, int rleStride, bool allowLog = true){
                //Validate
                if(rleData == nullptr){
                    if(allowLog) P_LOG_ERROR("Fed in nullptr as rle-source!");
                    return false;
                }
                if(validDatalength_bytes <= 0 || expectedOutputLength_bytes <= 0 || rleStride <= 0){	//Too small?
                    if(allowLog) P_LOG_ERROR("Data in invalid Range! (<= 0)");
                    return false;
                }
                if(expectedOutputLength_bytes > _maxDataSize || rleStride > _maxPacketSize || validDatalength_bytes > _maxDataSize){	//Too Large?
                    if(allowLog) P_LOG_ERROR("Input size too large!");
                    return false;
                }
                if(rleStride > validDatalength_bytes || rleStride > expectedOutputLength_bytes){				//Malformed?
                    if(allowLog) P_LOG_ERROR("Input-Data seems to be malformed~");
                    return false;
                }


                //Alloc 
                int rleIndex = 0;
                int amountOfBytesWritten = 0;
                unsigned char* intermediateRLEBuffer = new unsigned char[rleStride];
                destination.reserve(validDatalength_bytes);


                //Decompress
                while (rleIndex < validDatalength_bytes && amountOfBytesWritten < expectedOutputLength_bytes) {
                    //Prepare
                    unsigned char header = rleData[rleIndex];
                    rleIndex++;

                    //validate index
                    if(rleIndex >= validDatalength_bytes){
                        if(allowLog) P_LOG_ERROR("rleIndex out of bounds!");
                        delete[] intermediateRLEBuffer;
                        return false;
                    }
            
                    //Extract
                    bool headerIsFirstType = (0x80 & header) != 0;
                    int copies = static_cast<int>( (~0x80 & header) + 1 );
                    
                    //Validate size
                    if(copies == 0 || copies > _maxDataSize){
                        if(allowLog) P_LOG_ERROR(copies << " copies in RLE points to an error!");
                        delete[] intermediateRLEBuffer;
                        return false;
                    }

                    //'copies' times of repeated pixel data
                    if (headerIsFirstType) {
                        //Get RLE content
                        if(rleIndex >= validDatalength_bytes){
                            if(allowLog) P_LOG_ERROR("rleIndex out of bounds!");
                            delete[] intermediateRLEBuffer;
                            return false;
                        }

                        memcpy(intermediateRLEBuffer, &rleData[rleIndex], rleStride);	//safe
                        rleIndex += rleStride;
                        
                        //Decompress
                        for (int i = 0; i < copies; i++)
                        {
                            for (int n = 0; n < rleStride; n++) {
                                destination.push_back( intermediateRLEBuffer[n] );
                                amountOfBytesWritten++;
                            }
                        }
                    } else{ //'copies' amount of pixel-values
                        
                        //Decompress
                        for (int i = 0; i < copies; i++)
                        {
                            if(rleIndex >= validDatalength_bytes){
                                if(allowLog) P_LOG_ERROR("rleIndex out of bounds!");
                                delete[] intermediateRLEBuffer;
                                return false;
                            }
                            memcpy(intermediateRLEBuffer, &rleData[rleIndex], rleStride);	//safe
                            
                            for (int n = 0; n < rleStride; n++) {
                                destination.push_back( intermediateRLEBuffer[n] );
                                amountOfBytesWritten++;
                            }
                            
                            rleIndex += rleStride;
                        }
                    }
                }
                    


                //Cleanup
                delete[] intermediateRLEBuffer;

                //Validate Output
                if(destination.size() != expectedOutputLength_bytes){
                    if(allowLog) P_LOG_ERROR("Insufficient count of pixels where decompressed: " << destination.size() << "/" << expectedOutputLength_bytes);
                    return false;
                }

                return true;
            }


#ifdef DEBUG
        namespace RLETest{
            struct Args{
                std::vector<unsigned char>* destination;
                unsigned char* rle_data = nullptr;
                int rleLength_bytes = 0;
                int expectedOutputLength = 0;
                int rleStride = 0;
            };

            inline bool RunFullTest(){
                using namespace std;
                //utils
                auto DoTest = [](Args& arg, bool log){
                    return RLE_Decompress(*(arg.destination), arg.rle_data, arg.rleLength_bytes, arg.expectedOutputLength, arg.rleStride, log);
                };

                //Alloc Utils
                bool result = true;
                vector<unsigned char> intermediate = {};

                const int rleStride_bytes = 1;
                unsigned char inValidRLEData[] = { 0x8F };
                unsigned char validRLEData[] = {
                    0x80, 0x01,				//repeat 0x01 (0 + 1 = 1) time
                    0x01, 0xB3,	0xB4,		//take 0xB3 and 0xB4			
                };							//expected outcome-bytes: 3
                const int validDatalength_bytes = 5;

                unsigned char expectedOutput[] = {0x01, 0xB3, 0xB4};
                const int expectedOutput_bytes = 3;


                //Config valid test
                Args validArg = {&intermediate, validRLEData, validDatalength_bytes, expectedOutput_bytes, rleStride_bytes};

                //Valid Test
                bool testResult = DoTest(validArg, true);
                if(!P_UnitTest::CheckValidity(testResult, intermediate.data(), intermediate.size(), expectedOutput, expectedOutput_bytes)){
                    P_LOG_ERROR("Failed valid RLE test!");
                    result = false;
                }


                //Config invalid test
                const int invalidArgCount = 12;
                Args invalidArgs[invalidArgCount] = {};
                for(int i = 0; i < invalidArgCount; i++){
                    invalidArgs[i] = validArg;
                }

                invalidArgs[0].rle_data = nullptr;
                invalidArgs[1].rle_data = inValidRLEData;

                invalidArgs[2].rleLength_bytes = INT_MAX;
                invalidArgs[3].rleLength_bytes = 0;
                invalidArgs[4].rleLength_bytes = -1;

                invalidArgs[5].expectedOutputLength = INT_MAX;
                invalidArgs[6].expectedOutputLength = 0;
                invalidArgs[7].expectedOutputLength = -1;

                invalidArgs[8].rleStride = INT_MAX;
                invalidArgs[9].rleStride = 0;
                invalidArgs[10].rleStride = -1;

                invalidArgs[11].rleLength_bytes = invalidArgs[11].expectedOutputLength;

                //Run invalid test
                for(int i = 0; i < invalidArgCount; i++){
                    if(DoTest(invalidArgs[i], false)){
                        P_LOG_ERROR("Failed invalid RLE-Test at " << i);
                        result = false;
                    }
                }


                //Fin
                if(result) P_LOG_SUCCESS("RLE-Test successfull!");
                if(!result) P_LOG_ERROR("RLE-Test failed!");
            
                return result;
            }
        }

        //Run tests
        static_assert(_maxPacketSize > 0 && _maxPacketSize < 1024 * 1024, "invalid packet-size!");
        static_assert(_maxDataSize > 0 && _maxDataSize < 20 * 1024 * 1024, "invalid data-size!");
        inline P_UnitTest runCompressionTests = { {&RLETest::RunFullTest}, __FILE__ };  
#endif
    }
}