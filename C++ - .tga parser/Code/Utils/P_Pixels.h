#pragma once
#include <utility>
#include <vector>

#include "P_LogOut.h"
#include "P_UnitTest.h"



namespace Plum::Files{
	namespace P_Pixels{		

		enum class Order : char{
			RGBA = 0,
			BGRA = 1,
		};


		inline bool FlipImage(std::vector<unsigned char>& targetPixels, int bytesPerPixel, std::pair<int, int> resolution, std::pair<bool, bool> flipXY, bool allowLog = true) {
			using namespace std;
			//Utils
			auto Error = [targetPixels, bytesPerPixel, resolution, flipXY, allowLog](const char* msg){
				if(allowLog){
					P_LOG_ERROR(msg);
					P_LOG_ERROR("Passed args:");
					P_LOG_ERROR(targetPixels.size() << " = input-buffer length");
					P_LOG_ERROR(bytesPerPixel << " = bytes per pixel");
					P_LOG_ERROR(resolution.first << " x " << resolution.second << " = resolution");
					P_LOG_ERROR(flipXY.first << " x " << flipXY.second << " = FlipXY");
					P_LOG_ERROR("");
				}
				return false;
			};

			//Alias
			int width = resolution.first;
			int height = resolution.second;
			bool flipHorizontal = flipXY.first;
			bool flipVertical = flipXY.second;

			//Validate
			if(targetPixels.size() < width * height * bytesPerPixel || targetPixels.size() <= 0) return Error("Invalid Input-size!");
			if(bytesPerPixel > targetPixels.size() || bytesPerPixel <= 0) return Error("Invalid bytes per Pixel!");
			if(targetPixels.size() % bytesPerPixel != 0) return Error("Please ensure input-data to be valid!");
			if(width <= 0 || height <= 0) return Error("Invalid resolution!");

			//Valid run?
			if (!flipHorizontal && !flipVertical) return true;

			//Alloc
			vector<unsigned char> flippedImage = {};
			flippedImage.reserve(height * width * bytesPerPixel);

			vector<unsigned char> scanLineHorizontal(width * bytesPerPixel, 0x0);

			//Apply
			for (short y = 0; y < height; y++)
			{
				for (short x = 0; x < width; x++) {
					//Alloc
					int xOffset = flipHorizontal ? (width - 1 - x) : x;
					unsigned char* bufferDestination = &scanLineHorizontal[x * bytesPerPixel];
					unsigned char* bufferSource = &targetPixels[(y * width + xOffset) * bytesPerPixel];

					//Copy
					memcpy(bufferDestination, bufferSource, bytesPerPixel);
				}

				//Insert
				flippedImage.insert(flipVertical? flippedImage.begin() : flippedImage.end(), scanLineHorizontal.begin(), scanLineHorizontal.end());
			}
			

			//Finalize
			targetPixels.swap(flippedImage);
			return true;
		}
	
		inline bool GammaCorrect(std::vector<unsigned char>& imageData, float gamma = 2.2f, bool allowLog = true) {
			//https://en.wikipedia.org/wiki/Gamma_correction

			//Validate
			if(imageData.size() <= 0){
				if(allowLog) P_LOG_ERROR("Invalid imageData size: " << imageData.size());
				return false;
			}

			//Run
			for (int i = 0; i < imageData.size(); i++)
			{
				float v = static_cast<float>(imageData[i]) / 255.0f;
				float a = 1.0f;
				float y = gamma;
				float vOut = pow(v, y);
				imageData[i] = static_cast<unsigned char>(vOut * 255);
			}

			//End
			return true;
		}
		
		inline bool RearrangeColor32ToRGBA(std::vector<unsigned char>& pixels_color32, Order currentOrder, bool allowLog = true){
			//Utils
			auto Error = [pixels_color32, currentOrder, allowLog](const char* msg){
				if(allowLog){
					P_LOG_ERROR(msg);
					P_LOG_ERROR(pixels_color32.size() << " = pixels_color32.size()");
					P_LOG_ERROR((int)currentOrder << " = currentOrder");
				}
				return false;
			};
			static const int _bytesPerPixel = 4;

			//Validate
			if(pixels_color32.size() <= 0 || _bytesPerPixel <= 0) return Error("Please ensure valid data!");
			if(pixels_color32.size() < _bytesPerPixel) return Error("Fed-In PixelData-size too small");
			if(pixels_color32.size() % _bytesPerPixel != 0) return Error("Please ensure pixel-byte-count is divisible by 4 (expects 1 byte per pixel)");
	

			//Alloc;
			std::vector<unsigned char> rgbaPixels = {};
			rgbaPixels.reserve(pixels_color32.size());
		
			int offsetOfRed = 0;
			int offsetOfGreen = 1;
			int offsetOfBlue = 2;
			int offsetOfAlpha = 3;


			//Run
			for(int i = 0; i < pixels_color32.size(); i+=_bytesPerPixel){
				//Select offsets
				switch(currentOrder){
				case Order::BGRA:
					offsetOfBlue = 0;
					offsetOfGreen = 1;
					offsetOfRed = 2;
					offsetOfAlpha = 3;
				break;
					
				case Order::RGBA:
					return Error("Tried to convert RGBA into RGBA~ returning!");
				break;
					
				default:
					return Error("Warning: Got unknown color format when trying to rearrange image color channel order!");
				break;
				}
				
				//Append
				rgbaPixels.push_back( pixels_color32[i + offsetOfRed] );
				rgbaPixels.push_back( pixels_color32[i + offsetOfGreen] );
				rgbaPixels.push_back( pixels_color32[i + offsetOfBlue] );
				rgbaPixels.push_back( pixels_color32[i + offsetOfAlpha] );
			}

			//Finalize
			pixels_color32.swap(rgbaPixels);
			return true;
		}
	


#ifdef DEBUG
	namespace FlipTest{
		//Utils
		struct Args{
			std::vector<unsigned char>& srcBuffer;
			int bytesPerPixel = 0;
			std::pair<int, int> resolution = {0, 0};
			std::pair<bool, bool> flipXY = {0, 0};
		};

		inline bool RunFullTest(){
			using namespace std;

			auto DoTest = [](Args& arg, bool log){
				return FlipImage(arg.srcBuffer, arg.bytesPerPixel, arg.resolution, arg.flipXY, log);
			};

			//Alloc
			bool result = true;
			vector<unsigned char> intermediate = {};
			
			//Config
			intermediate.clear();
			intermediate.push_back(0xFF);
			intermediate.push_back(0x00);
			Args validTest = { intermediate, 1, {1, 2}, {true, true} };

			//Run
			vector<unsigned char> expectedData = {0x00, 0xFF};
			bool validResult = DoTest(validTest, false);
			if( !P_UnitTest::CheckValidity(validResult , intermediate.data(), intermediate.size(), expectedData.data(), expectedData.size())){
				result = false;
				P_LOG_ERROR("Failed valid Flip-Test!");
			}

			//Invalid-Test
			vector<Args> invalidArgs = {
				{ intermediate, 0, {0, 0}, {false, false} },
				{ intermediate, 0xFF, {1, 1}, {false, false} },
				{ intermediate, 1, {0, 1}, {false, false} },
				{ intermediate, 1, {1, 0}, {false, false} },
			};

			//Run
			for(int i = 0; i < invalidArgs.size(); i++){
				intermediate.clear();
				if(i > 0) intermediate.push_back(0xFF);	//First test for empty-source

				if(DoTest(invalidArgs[i], false)){
					P_LOG_ERROR("Failed at invalid test: " << i);
					result = false;
				}
			}

			//Finalize
			if(result) P_LOG_SUCCESS("P_Pixels Flip passed Successfully!");
			if(!result) P_LOG_ERROR("Failed Flip-Test!");
			return result;
		}
	}

	namespace RearrangeTest{

		struct Args{
			std::vector<unsigned char>& pixels_color32;
			Order currentOrder = Order::BGRA;
		};

		inline bool DoTest(Args& arg, bool log){
			return RearrangeColor32ToRGBA(arg.pixels_color32, arg.currentOrder, log);
		}

		inline bool RunFullTest(){
			using namespace std;

			//Alloc 
			vector<unsigned char> intermediate = {};
			bool result = true;

			//Config valid test
			intermediate.push_back(0x30);
			intermediate.push_back(0x20);
			intermediate.push_back(0x10);
			intermediate.push_back(0x00);

			unsigned char expectedData[] = {0x10, 0x20, 0x30, 0x00};
			int expectedByteCount = 4;
			Args validTest = { intermediate, Order::BGRA };

			//Run valid test
			bool validResult = DoTest(validTest, true);
			if( !P_UnitTest::CheckValidity(validResult, intermediate.data(), intermediate.size(), expectedData, expectedByteCount) ){
				P_LOG_ERROR("Failed ")
			}


			//Config invalid test
			Args invalidArgs[] = {
				{intermediate, Order::BGRA},	//invalid input data
				{intermediate, Order::RGBA},
				{intermediate, static_cast<Order>( char(0xFF) )}
			};
			const int invalidArgCount = 3;
			
			//Run invalid test
			intermediate.clear();
			intermediate.push_back(0x00);
			for(int i = 0; i < invalidArgCount; i++){
				if(i > 0){
					intermediate.clear();
					intermediate.push_back(0x10);
					intermediate.push_back(0x20);
					intermediate.push_back(0x30);
					intermediate.push_back(0x00);
				}

				if(DoTest(invalidArgs[i], false)){
					P_LOG_ERROR("Failed invalid test at: " << i);
					result = false;
				}
			}

			if(result) P_LOG_SUCCESS("Passed P_Pixels Rearrange Test successfully!");
			if(!result) P_LOG_ERROR("Failed P_Pixels Rearrange Test!");

			result = false;
			return result;
		}
	}

	//Run Tests
	inline P_UnitTest runPixelTests = { { &FlipTest::RunFullTest, &RearrangeTest::RunFullTest }, __FILE__};
#endif
	};
}