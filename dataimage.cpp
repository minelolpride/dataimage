#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>

// This header file was pulled from somewhere on GitHub, and it is quite a miracle that I found it.
#include "bitmap_image.hpp"

// used to use this so fopen() was possible
// #pragma warning(disable : 4996)

using namespace std;

int main(void) {
	
	// init stuff
	fstream inputData;		// our input file
	string inputName;		// the name of our input file, taken from the user
	unsigned long long inputOffset;		// the byte offset to start at, taken from the user
	string dataFormat;		// how we are going to interpret the input data
	unsigned long long outW;				// output image dimensions
	unsigned long long outH;				// see above
	unsigned long long outA;				// total pixels of the image, compared against below to see if we have enough data
	streampos fileS = -1;	// amount of data in the file, initialized to -1 for error checking later
	unsigned long long dataNeeded;			// amount of data needed here

	// get the filename so we know what to open, then open
	do {
		cout << "What is the input file? (including extension, if any)" << endl;
		getline(cin, inputName);
		//cin >> inputName;
		inputData.open(inputName, ios::in | ios::binary | ios::ate);

		// get the total size of the input file
		fileS = inputData.tellg(); // we used ios::ate when opening so we dont need to re-seek to the end.
		if (fileS == -1) {
			cout << "That doesn't look right... Try again." << endl;
		}
	} while (fileS == -1);
	cout << "Size of input data: " << (unsigned long long)fileS << endl;
	// if you don't provide a file that exists or you make a mistake while typing this will come back with -1 length.

	// i tried putting this in a while loop to make sure it doesnt escape if the size is -1
	// but VS2017 will literally not stop zinging me for 2 seconds
	// and it probably will try not to build if i put these comments
	// this machine works in mysterious ways.

	// get the byte offset so we know where to start looking
	cout << "How far in the file should I look? ";
	cin >> inputOffset;
	if (inputOffset < 0) {
		cout << "You gave me an offset under 0, so I will use 0." << endl;
		inputOffset = 0;
	}
	inputData.seekg(inputOffset); // go where we are told to

	// this is interesting, heres how this all goes down
	//		rgbi will grab the data and use it like [RGBRGBRGBRGBRGB] throughout the file
	//		rgbs will grab the data and use it like [RRRRRGGGGGBBBBB] throughout the file
	//		bw just eats each byte as a pixel that is white or black or anywhere between those.
	cout << "How should I interpret the data? (rgbi rgbs bw) ";
	cin >> dataFormat;
	// i would loop this until it gets an input that matches what it can use
	// but for some reason itll always get stuck in that loop with no escape. weird.

	// get the desired image dimensions for the output
	// they dont have a reasonable upper bound, but they definetly can't be negative
	cout << "Image dimensions (Width) ";
	cin >> outW;
	if (outW <= 0) {
		cout << "Flooring output width to 1..." << endl;
		outW = 1;
	}

	cout << "Image dimensions (Height) ";
	cin >> outH;
	if (outH <= 0) {
		cout << "Flooring output height to 1..." << endl;
		outH = 1;
	}

	// set the total size of the output image
	// used to see if we have enough data to make image
	outA = outW * outH;


	// i think i know how im going to optimize this to fix the input file size speed issue
	// i load the file in its entirety first, but when i load the stuff into the stringstream, i only load what i need
	// this means we should have better consistency and speed when generating

	// though we will have to know if we have enough data NOW instead of later
	// i hope to god .read() works

	// check how much data we need
	if (dataFormat == "rgbi") dataNeeded = outA * 3;
	if (dataFormat == "rgbs") dataNeeded = outA * 3;
	if (dataFormat == "bw") dataNeeded = outA;

	// check how much bytes we have available
	unsigned long long dataAvailable = (unsigned long long)fileS - inputOffset;
	cout << "Available bytes: " << dataAvailable << "/" << dataNeeded << endl;

	// compare to see if we have enough data
	if (dataNeeded > dataAvailable) {
		// we dont
		cout << "I don't have enough data to make the image!" << endl;
		cout << "I need " << dataNeeded << " bytes, but I only have " << dataAvailable << " bytes available." << endl;
		cout << "You can try using a larger file, moving the offset back, lowering the target resolution, or using a different data format." << endl;
		cout << "If you believe this message has appeared in error, that means I probably suck at coding." << endl;
		return 1;
	}

	char* inputMemory = new char[dataNeeded];
	inputData.read(inputMemory, dataNeeded);
	//cout << inputMemory << endl;




	// read all chars from file into memory
	// YOU HAVE NO IDEA HOW LONG IT TOOK ME TO DO FIGURE THIS ONE OUT
	// GOD.
	//stringstream inputMemory;
	//inputMemory << inputData.rdbuf();
	//inputMemory.clear();
	
	// USE THIS TO READ THE STUFF
	// (int)inputMemory.str().c_str()[n]

	// prepare the output canvas
	// this is where the magic goop gets added to the mix
	bitmap_image outputFile(outW, outH);
	if (!outputFile) {
		cout << "cannot open output";
		return 1;
	};

	cout << "Generating image. Please wait..." << endl;

	// now we generate the image

	// couple things to note right now with this
	//  1. its single-threaded so its gonna take fucking forever to work it's magic
	//  2. it gets slower the bigger the input file you give it, you probably don't want to go over 10 MB
	//  3. because of how making grayscale images be, it should be way faster since it only accesses one byte of the file for each pixel instead of 3

	if (dataFormat == "rgbi") {
		// We need 3 bytes for every pixel, so we need 3*outA to fulfill the task
		dataNeeded = outA * 3;
		// check if we have enough data to work with
		if (dataNeeded > dataAvailable) {
			// we dont
			cout << "I don't have enough data to make the image!" << endl;
			cout << "I need " << dataNeeded << " bytes, but I only have " << dataAvailable << " bytes available." << endl;
			cout << "You can try using a larger file, moving the offset back, lowering the target resolution, or using a different data format." << endl;
			cout << "If you believe this message has appeared in error, that means I probably suck at coding." << endl;
			return 1;
		} else {
			// we doooooo
			unsigned long long i = inputOffset;	// set the offset so we can start
			unsigned long long curX = 0;			// init our current x position
			unsigned long long curY = 0;			// init our current y position
			while (curY < outH) {
				while (curX < outW) {
					// set the pixel at the current coordinates using grouped triplets of bytes
					//outputFile.set_pixel(curX, curY, (int)inputMemory.str().c_str()[i], (int)inputMemory.str().c_str()[i + 1], (int)inputMemory.str().c_str()[i + 2]);
					outputFile.set_pixel(curX, curY, (unsigned long long)inputMemory[i], (unsigned long long)inputMemory[i + 1], (unsigned long long)inputMemory[i + 2]);
					// incremenmnemnt our current offset to the next triplet
					i += 3;
					curX++;
				}
				// we output each time we complete a row just so we can be super sure that its working
				//cout << "Row " << curY << " done." << endl;
				curY++;
				curX = 0;
			}
		}
	}


	if (dataFormat == "rgbs") {
		dataNeeded = outA * 3;
		if (dataNeeded > dataAvailable) {
			cout << "I don't have enough data to make the image!" << endl;
			cout << "I need " << dataNeeded << " bytes, but I only have " << dataAvailable << " bytes available." << endl;
			cout << "You can try using a larger file, moving the offset back, lowering the target resolution, or using a different data format." << endl;
			cout << "If you believe this message has appeared in error, that means I probably suck at coding." << endl;
			return 1;
		}
		else {
			unsigned long long i = inputOffset;
			unsigned long long curX = 0;
			unsigned long long curY = 0;
			while (curY < outH) {
				while (curX < outW) {
					//outputFile.set_pixel(curX, curY, (int)inputMemory.str().c_str()[i], (int)inputMemory.str().c_str()[i + outA], (int)inputMemory.str().c_str()[i + outA + outA]);
					outputFile.set_pixel(curX, curY, (unsigned long long)inputMemory[i], (unsigned long long)inputMemory[i + outA], (unsigned long long)inputMemory[i + outA + outA]);
					i++;
					curX++;
				}
				//cout << "Row " << curY << " done." << endl;
				curY++;
				curX = 0;
			}
		}
	}

	if (dataFormat == "bw") {
		unsigned long long pxc; // for later, see the part where we plot pixels
		// We need 1 byte for each pixel, and we dont need to move it unneccesarily
		if (outA > dataAvailable) {
			cout << "I don't have enough data to make the image!" << endl;
			cout << "I need " << outA << " bytes, but I only have " << dataAvailable << " bytes available." << endl;
			cout << "You can try using a larger file, moving the offset back, lowering the target resolution, or using a different data format." << endl;
			cout << "If you believe this message has appeared in error, that means I probably suck at coding." << endl;
			return 1;
		}
		else {
			unsigned long long i = inputOffset;
			unsigned long long curX = 0;
			unsigned long long curY = 0;
			while (curY < outH) {
				while (curX < outW) {
					// grab the byte and put it to memory
					//pxc = (int)inputMemory.str().c_str()[i];
					pxc = (unsigned long long)inputMemory[i];
					outputFile.set_pixel(curX, curY, pxc, pxc, pxc); // and use the thing in memory to reduce the amount of times we access the file
					i++; // next byte
					curX++;
				}
				//cout << "Row " << curY << " done." << endl;
				curY++;
				curX = 0;
			}
		}
	}

	// we get here only after finishing the pixels, lets get the file out!
	outputFile.save_image("output.bmp");
	inputData.close();

	return 0;
}
