#pragma once
#include <stdio.h>
#include <cstdint>
#include <vector>
#include <string>
#include <limits>

#include "Utils/P_LogOut.h"
#include "Utils/P_UnitTest.h"

#include "Utils/P_Pixels.h"
#include "Utils/P_Compression.h"

//https://www.fileformat.info/format/tga/egff.htm#TGA-DMYID.3
//TGA FILE Structure:
//Header (18 bytes)
//ID (optional var)
//Color Map (optional var)
//Image Data (var)
//=== Optional developer area in newer format (ignored here) ===


namespace Plum::Files{

class P_TGAFile {
public:
	inline static const int _maxPixelBytes = 1 * 1024 * 1024;	//1mb

public:
#pragma pack(push, 1)
	struct Header {
		friend class P_TGAFile;

	public:
		enum class ImageType : int8_t {
			EMPTY = 0,
			UNCOMPRESSED_COLORMAPPED = 1,
			UNCOMPRESSED_TRUECOLOR = 2,
			UNCOMPRESSED_greyScale = 3,
			RLE_COLORMAPPED = 9,
			RLE_TRUECOLOR = 10,
			RLE_greyScale = 11
		};

		enum class Origin : char {
			BOTTOM_LEFT = 0,
			BOTTOM_RIGHT = 1,
			TOP_LEFT = 2,
			TOP_RIGHT = 3,
		};


	public:
		//File specification
		int8_t idLength = 0;
		int8_t colorMapType = 0;
		ImageType imageType = ImageType::EMPTY;

		//Color map specifications
		int16_t colorMapStartIndex = 0;
		int16_t colorMapLength = 0;
		int8_t colorMapBitDepth = 0;

		//Image specifications
		int16_t xOrigin = 0;	
		int16_t yOrigin = 0;
		int16_t width = 0;
		int16_t height = 0;
		int8_t bitsPerPixel = 0;
		int8_t imageDescriptor = 0;


	private:
		inline void ReverseEndianness(){
			using namespace std;
			P_LOG_INFO("Reversing Endianness!");

			auto ReVar = [](auto* var){ reverse(var, var + 1); };

			ReVar(&colorMapStartIndex);
			ReVar(&colorMapLength);
			ReVar(&xOrigin);
			ReVar(&yOrigin);
			ReVar(&width);
			ReVar(&height);
		}

	public:
		inline std::pair<int, int> GetRez() { return { width, height }; }
		inline int GetPixelDepth_byte() { return bitsPerPixel / 8; }
		inline int GetFullImageSize_byte() { return width * height * GetPixelDepth_byte(); }
		inline int GetColorMapLength_byte() { return colorMapLength * (colorMapBitDepth / 8); }	
		inline unsigned char GetAlphaChannelSize() { return 0x0F & imageDescriptor; }
	};
#pragma pack(pop)



public:
	inline P_TGAFile(char* nullTerminatedDirectory){
		using namespace std;

		FILE* file = fopen(nullTerminatedDirectory, "rb");
		if(file == nullptr){
			P_LOG_ERROR("Cannot open .tga file: " << nullTerminatedDirectory);
			return;
		}


		if(fseek(file, 0, SEEK_END) != 0){
			P_LOG_ERROR("Failed fseek!");
			fclose(file);
			return;
		}

		int fileSize = ftell(file);
		if(fileSize > _maxPixelBytes){
			P_LOG_ERROR("Max allowed file-size: " << _maxPixelBytes << " bytes!");
			fclose(file);
			return;
		}

		if(fseek(file, -fileSize, SEEK_END) != 0){
			P_LOG_ERROR("Failed to reset file-pointer!");
			fclose(file);
			return;
		}

		char* tgaBuffer = new char[fileSize];
		if(fread(tgaBuffer, sizeof(char), fileSize, file) != fileSize){
			P_LOG_ERROR("Failed to read file!");
			fclose(file);
			return;
		}


		if( !LoadFromBuffer(tgaBuffer, fileSize) ) P_LOG_ERROR("Failed to load .tga File!");

		delete[] tgaBuffer;	
		fclose(file);
	}

	inline P_TGAFile(char* tgaSourceBytes, const int tgaByteCount) { 
		if( !LoadFromBuffer(tgaSourceBytes, tgaByteCount) )  P_LOG_ERROR("Failed to load .tga File!");
	}

	inline ~P_TGAFile(){ }


private:
	inline bool LoadFromBuffer(char* tgaSourceBytes, const int tgaLength_byte) {	
		using namespace std;
		P_LOG_INFO("Loading TGA File from " << (int*)tgaSourceBytes << ", tgaLength_byte: " << tgaLength_byte);


		//Validate
		if(tgaSourceBytes == nullptr){
			P_LOG_ERROR("Please load valid .tga bytes, not nullptr!");
			return false;
		}

		if(tgaLength_byte <= sizeof(Header)){
			P_LOG_ERROR("Please ensure the loaded Data is a .tga File!");
			return false;
		}

		//Check Endianness
		int n = 1;
		char* nAddress = reinterpret_cast<char*>(&n);
		bool systemIsBigEndian = nAddress[0] != 1;

		//Get Header
		memcpy(&header, tgaSourceBytes, sizeof(Header));

		//Flip Bytes?
		if(systemIsBigEndian) {
			P_LOG_INFO("Reversing Header-Endianness");
			header.ReverseEndianness();
		}

		//Validate Type
		if(header.imageType <= Header::ImageType::EMPTY || header.imageType > Header::ImageType::RLE_greyScale){
			P_LOG_ERROR("Unsupported Type: " << static_cast<int>(header.imageType));
			return false;
		}


		//GET IMAGE DATA
		int imageDataOffset = sizeof(Header) + header.idLength + header.GetColorMapLength_byte();

		//Confirm	
		if (imageDataOffset > tgaLength_byte) {
			P_LOG_ERROR("Tried to read .tga file with buffer overflow");
			return false;
		}

		//Load image data
		decoded.clear();
		unsigned char* rawImageData = reinterpret_cast<unsigned char*>(&tgaSourceBytes[imageDataOffset]);

		//RLE Decompress
		if ( header.imageType > Header::ImageType::UNCOMPRESSED_greyScale ) {
			P_LOG_INFO("Decompressing image-Data");

			int pixelCount_bytes = header.width * header.height * header.GetPixelDepth_byte();
			int rleStride = header.GetPixelDepth_byte();
			decoded.reserve(pixelCount_bytes);
			if( !P_Compression::RLE_Decompress(decoded, rawImageData, tgaLength_byte, pixelCount_bytes, rleStride) ) return false;
		}	
		else {	//use raw data
			P_LOG_INFO("Using raw image-data, no compression");
			decoded.insert(decoded.begin(), rawImageData, rawImageData + GetpixelDataBytes());
		}
		

		//CONVERT
		int bitsPerPixel = header.bitsPerPixel;

		//Use color-map?
		if (header.colorMapLength > 0 && header.colorMapType > 0){ 
			P_LOG_INFO("Loading colormap");

			//Alloc
			int colorMapBytes = header.GetColorMapLength_byte();
			int colorMapOffset = sizeof(Header) + header.idLength;

			if (colorMapOffset + colorMapBytes > tgaLength_byte || colorMapOffset > tgaLength_byte) {
				P_LOG_ERROR("Tried to construct tga colormap with buffer overflow");
				return false;
			}
			
			if(colorMapBytes > _maxPixelBytes){
				P_LOG_ERROR("Colormap too large!");
				return false;
			}

			unsigned char* colorMap = reinterpret_cast<unsigned char*>(&tgaSourceBytes[colorMapOffset]);
			if(!ApplyColorMap(decoded, colorMap, colorMapBytes, (int)header.colorMapBitDepth / 8, (int)header.colorMapStartIndex)){
				P_LOG_ERROR("Failed to apply colormap!");
				return false;
			}

			bitsPerPixel = header.colorMapBitDepth;
		}

		//Finalize Pixel-Data to BGRA8 (32 bits)
		switch (bitsPerPixel)
		{
			case 8:
				P_LOG_INFO("Converting from 8 to 32 bit pixel-depth");
				Expand8To32Bit(decoded);
				break;

			case 16:
				P_LOG_INFO("Converting from 16 to 32 bit pixel-depth");
				Expand16To32Bit(decoded, header.GetAlphaChannelSize() > 0);
				break;

			case 24:
				P_LOG_INFO("Adding Alpha channel to BGR8");
				Expand24To32Bit(decoded);
				break;

			case 32:
				P_LOG_INFO("Expected pixel-depth of 32");
				break;

			default:
				P_LOG_ERROR("Unexpected TGA pixel-depth of: " << bitsPerPixel);
				break;
		}
		bitsPerPixel = 32;

		//Finalize Image
		P_TGAFile::Header::Origin imageOrigin = GetOrigin();
		bool shouldFlipVertically = imageOrigin == P_TGAFile::Header::Origin::TOP_LEFT || imageOrigin == P_TGAFile::Header::Origin::TOP_RIGHT;
		bool shouldFlipHorizontally = imageOrigin == P_TGAFile::Header::Origin::TOP_RIGHT || imageOrigin == P_TGAFile::Header::Origin::BOTTOM_RIGHT;
		
		int bytesPerPixel = bitsPerPixel / 8;
		P_Pixels::FlipImage(decoded, bytesPerPixel, {header.width, header.height}, {shouldFlipHorizontally, shouldFlipVertically} );
		P_Pixels::RearrangeColor32ToRGBA(decoded, P_Pixels::Order::BGRA);

		//Finalize
		P_LOG_SUCCESS("Read P_TGA!");
		return true;
	}


public:
	//TODO: Preallocated, 'pixels' and 'intermediate' vectors (both with max-size = .reserve(width * height * 4)) per .tga would suffice and be more heap friendly.

	//in: {index, ...} ; out: {(bytesPerPixel amount of bytes), ...}
	inline static bool ApplyColorMap(std::vector<unsigned char>& imageData, unsigned char* colorMap, int colorMapSize_bytes, int bytesPerPixel, int startingIndex, bool allowLog = true){
		//Utils
		auto Error = [allowLog](const char* msg){
			if(allowLog) P_LOG_ERROR(msg);
			return false;
		};

		//Validate
		if(colorMap == nullptr) return Error("ColorMap was nullptr!");
		if(imageData.size() <= 0 || colorMapSize_bytes <= 0 || bytesPerPixel <= 0 || startingIndex < 0) return Error("Size too small!");
		if(imageData.size() > _maxPixelBytes || colorMapSize_bytes > _maxPixelBytes || bytesPerPixel > _maxPixelBytes || startingIndex > _maxPixelBytes) return Error("Data too large!");
		if(startingIndex > colorMapSize_bytes) return Error("Malformed Data?");
		
		//Alloc
		std::vector<unsigned char> finalColorData = {};
		finalColorData.reserve(imageData.size() * bytesPerPixel);

		//Apply Map
		for (int i = 0; i < imageData.size(); i++)
		{
			//Get indices
			int index = startingIndex + static_cast<int>(imageData[i]);
			int colorMapIndex = index * bytesPerPixel;

			//Validate
			if (colorMapIndex + bytesPerPixel > colorMapSize_bytes || colorMapIndex > colorMapSize_bytes) {
				return Error("Tried to construct image data from colormap with buffer overflow");
			}

			//Apply by inserting color from map
			finalColorData.insert(finalColorData.end(), &colorMap[colorMapIndex], &colorMap[colorMapIndex + bytesPerPixel]);
		}

		//Finalize
		imageData.swap(finalColorData);
		return true;
	}

	//in: {byte} ; out: {byte, byte, byte, 255, ...}
	inline static bool Expand8To32Bit(std::vector<unsigned char>& imageGreyscale, bool allowLog = true){
		//Validate
		if(imageGreyscale.size() > _maxPixelBytes || imageGreyscale.size() <= 0){
			if(allowLog) P_LOG_ERROR("invalid single byte input size: " << imageGreyscale.size());
			return false;
		}

		//Alloc
		std::vector<unsigned char> output = {};
		output.reserve(imageGreyscale.size() * 4);
		
		//Decode
		for (int i = 0; i < imageGreyscale.size(); i++)
		{
			output.push_back(imageGreyscale[i]);
			output.push_back(imageGreyscale[i]);
			output.push_back(imageGreyscale[i]);
			output.push_back(255);
		}

		//Finalize
		imageGreyscale.swap(output);
		return true;
	}

	//in (little endian): {MSB 1 bit (Alpha) 5 bits (r) 5 bits(g) 5 bits(b), ...} ; out: {byteB, byteG, byteR, byteA, ...}
	inline static bool Expand16To32Bit(std::vector<unsigned char>& rawImageData, bool useAlpha, bool allowLog = true){
		//Utils
		auto Error = [allowLog](const char* msg){
			if(allowLog) P_LOG_ERROR(msg);
			return false;
		};

		//Validate
		if(rawImageData.size() > _maxPixelBytes || rawImageData.size() <= 0) return Error("Invalid 16-bit image data size!");
		if(rawImageData.size() % 2 != 0) return Error("16-bit/2-byte data-length should be divisible by 2~");

		//Alloc
		std::vector<unsigned char> uncompressedColors = {};
		uncompressedColors.reserve(rawImageData.size() * 2);
		
		//Convert
		for (int i = 0; i < rawImageData.size(); i+=2)
		{	
			//Load pixel
			short pixel = 0x0;
			memcpy(&pixel, &rawImageData[i], 2);	//(little endian data)

			//convert to byte by (value & 5bitmask >> shiftToByte)
			unsigned char a = useAlpha? (pixel & 0x8000) >> (3 + 4 + 4 + 4) : 1;		//first bit = alpha
			unsigned char r = (pixel & 0x7C00) >> (4 + 4 + 2);						//following 5 bits = r
			unsigned char g = (pixel & 0x03E0) >> (4 + 1);							//following 5 bits = g
			unsigned char b = (pixel & 0x001F);										//following 5 bits = b

			//map to full byte range
			uncompressedColors.push_back(b << 3 | b >> 2);
			uncompressedColors.push_back(g << 3 | g >> 2);
			uncompressedColors.push_back(r << 3 | r >> 2);
			uncompressedColors.push_back( (a > 0)? 255 : 0 );
		}

		//Finalize
		rawImageData.swap(uncompressedColors);
		return true;
	};

	//in: {b, g, r, ...} ; out: {b, g, r, a, ...}
	inline static bool Expand24To32Bit(std::vector<unsigned char>& rawImageData, bool allowLog = true){
		//Utils
		auto Error = [allowLog](const char* msg){
			if(allowLog) P_LOG_ERROR(msg);
			return false;
		};

		//Validate
		if(rawImageData.size() > _maxPixelBytes || rawImageData.size() <= 0) return Error("Invalid 24-Bit size!");
		if(rawImageData.size() % 3 != 0) return Error("24-Bit/3-Byte data length should be divisible by 3~");

		//Alloc
		std::vector<unsigned char> withAlpha = {};
		withAlpha.reserve(rawImageData.size() + (rawImageData.size() / 3));

		//Convert
		for (int i = 0; i < rawImageData.size(); i++)
		{
			withAlpha.push_back(rawImageData[i]);
			if ((i + 1) % 3 == 0) withAlpha.push_back(255);	//just adds alpha-channel
		}

		//Finalize
		rawImageData.swap(withAlpha);
		return true;
	};


private:
	Header header = {};
	std::vector<unsigned char> decoded = {};


public:
	inline std::vector<unsigned char>& GetRawPixels_RGBA() { return decoded; }
	inline int GetpixelDataBytes() { return header.width * header.height * header.GetPixelDepth_byte(); }
	inline Header::Origin GetOrigin(){ return (Header::Origin)((header.imageDescriptor >> 4) & 0x03); }
	inline std::pair<int, int> GetRez(){ return {header.width, header.height}; }


#ifdef DEBUG
public:
	inline void Log() {
		P_LOG_SPACE();
		P_LOG_INFO("TGA File");

		P_LOG_INFO("Width: " << header.width);
		P_LOG_INFO("Height: " << header.height);
		P_LOG_INFO("Compression Type: " << static_cast<int>(header.imageType));
		P_LOG_INFO("Color depth: " << header.GetPixelDepth_byte());
		P_LOG_INFO("Color map depth: " << header.colorMapBitDepth);

		P_LOG_INFO("TGA File-End");
	}
#endif
};



//TESTS
#ifdef DEBUG
namespace TGATest{

namespace Colormap{

	struct Args{
		std::vector<unsigned char>* intermediate;
		unsigned char* colorMap = nullptr;
		int colorMapLength_bytes = 0;
		int colorDepth_bytes = 0;
		int offset = 0;
	};


	inline bool RunFullTest(){
		//Utils
		auto DoTest = [](Args& arg, bool log){
			return P_TGAFile::ApplyColorMap(*(arg.intermediate), arg.colorMap, arg.colorMapLength_bytes, arg.colorDepth_bytes, arg.offset, log);
		};

		//Alloc
		bool result = true;
		std::vector<unsigned char> intermediate = {};
		
		unsigned char validColorMap[] = {
			0xFF, 0x0F, 0x00, 0xFF,
		};
		unsigned char invalidColorMap[] = {
			0x00
		};

		const int validCMapSize_bytes = 4;
		const int cMapDepth_bytes = 4;
		const int expectedOutputPixelCount = 1;
		const int cMapIndex = 0;


		//Config valid test
		intermediate.clear();
		intermediate.push_back(0);
		Args validArg = {&intermediate, validColorMap, validCMapSize_bytes, cMapDepth_bytes, cMapIndex};

		//Run valid test
		bool testResult = DoTest(validArg, false);
		result = P_UnitTest::CheckValidity(testResult, intermediate.data(), intermediate.size(), validColorMap, validCMapSize_bytes);


		//Config invalid test
		const int invalidArgCount = 8;
		Args invalidArgs[invalidArgCount] = {};
		for(int i = 0; i < invalidArgCount; i++){
			invalidArgs[i] = validArg;
		}

		intermediate.clear(); //teting empty buffer in 0
		invalidArgs[1].colorMap = nullptr;

		invalidArgs[2].colorMapLength_bytes = INT_MAX;
		invalidArgs[3].colorMapLength_bytes = -1;

		invalidArgs[4].colorDepth_bytes = INT_MAX;
		invalidArgs[5].colorDepth_bytes = -1;

		invalidArgs[6].offset = INT_MAX;
		invalidArgs[7].offset = -1;

		//Run invalid test
		for(int i = 0; i < invalidArgCount; i++){
			if(DoTest(invalidArgs[i], false)){
				P_LOG_ERROR("Failed invalid test at: " << i);
				result = false;
			}

			intermediate.clear();
			intermediate.push_back(0);
		}

		return P_UnitTest::EvaluateResult(result, ".tga colormap");
	}
	
}


namespace ColorDepth{
	
	inline bool Test8To32Bit(){
		//Alloc
		bool result = true;
		std::vector<unsigned char> intermediate = {};

		//8 TO 32 BIT
		//Config valid test
		intermediate.clear();
		intermediate.push_back(0xF1);
		intermediate.push_back(0xF1);
		unsigned char expectedData[] = {
			0xF1, 0xF1, 0xF1, 255,
			0xF1, 0xF1, 0xF1, 255,
		};
		const int expectedSize = 8;

		//Run valid test
		bool vResult = P_TGAFile::Expand8To32Bit(intermediate);
		if(!P_UnitTest::CheckValidity(vResult, intermediate.data(), intermediate.size(), expectedData, expectedSize)){
			P_LOG_ERROR("Failed 8 to 32 bit valid test!");
			result = false;
		}

		//Invalid test
		intermediate.clear();
		if(P_TGAFile::Expand8To32Bit(intermediate, false)) {
			P_LOG_ERROR("Failed invalid test on 8 to 32 bit!");
			result = false;
		}

		return P_UnitTest::EvaluateResult(result, ".tga 8 to 32 bit color conversion");
	}

	inline bool Test16To32Bit(){
		//Alloc
		bool result = true;
		std::vector<unsigned char> intermediate = {};

		//Config valid test
		intermediate.push_back(0xFF);	
		intermediate.push_back(0x7F);	//little endian

		unsigned char expectedData[] = {
			0xFF, 0xFF, 0xFF, 0x00
		};
		const int expectedSize = 4;

		//Run valid test
		bool vResult = P_TGAFile::Expand16To32Bit(intermediate, true);
		result = P_UnitTest::CheckValidity(vResult, intermediate.data(), intermediate.size(), expectedData, expectedSize);

		//Invalid Test
		intermediate.clear();
		intermediate.push_back(0x0F);
		if(P_TGAFile::Expand16To32Bit(intermediate, true, false)) {
			P_LOG_ERROR("Failed invalid test!");
			result = false;
		}

		return P_UnitTest::EvaluateResult(result, ".tga 16 to 32 bit");
	}

	inline bool Test24To32Bit(){
		//Alloc
		bool result = true;
		std::vector<unsigned char> intermediate = {};
		
		//Config valid test
		intermediate.clear();
		intermediate.push_back(0x0F);
		intermediate.push_back(0xF0);
		intermediate.push_back(0xFF);

		unsigned char expectedData[] = {
			0x0F,
			0xF0,
			0xFF,
			255
		};
		const int expectedSize = 4;

		//Run valid test
		bool vResult = P_TGAFile::Expand24To32Bit(intermediate);
		result = P_UnitTest::CheckValidity(vResult, intermediate.data(), intermediate.size(), expectedData, expectedSize);


		//Run invalid test
		intermediate.clear();
		intermediate.push_back(0x0F);
		intermediate.push_back(0x0F);
		intermediate.push_back(0x0F);
		intermediate.push_back(0x0F);

		if(P_TGAFile::Expand24To32Bit(intermediate, false)) {
			P_LOG_ERROR("Failed invalid test!");
			result = false;
		}


		return P_UnitTest::EvaluateResult(result, ".tga 24 to 32 bit");
	}
}


	//Run Tests
	static_assert(sizeof(P_TGAFile::Header) == 18, "Incorrect .tga header-size!");
	static_assert(P_TGAFile::_maxPixelBytes > 0 && P_TGAFile::_maxPixelBytes < 20 * 1024 * 1024, "Invalid _maxPixelbytes (max. 20MB per Image)");

	inline P_UnitTest tgaTest = { {&Colormap::RunFullTest, &ColorDepth::Test8To32Bit, &ColorDepth::Test16To32Bit, &ColorDepth::Test24To32Bit}, __FILE__ };

}
#endif
}