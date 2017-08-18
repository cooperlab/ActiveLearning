//
//	Copyright (c) 2014-2017, Emory University
//	All rights reserved.
//
//	Redistribution and use in source and binary forms, with or without modification, are
//	permitted provided that the following conditions are met:
//
//	1. Redistributions of source code must retain the above copyright notice, this list of
//	conditions and the following disclaimer.
//
//	2. Redistributions in binary form must reproduce the above copyright notice, this list
// 	of conditions and the following disclaimer in the documentation and/or other materials
//	provided with the distribution.
//
//	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//	SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
//	WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//	DAMAGE.
//

//
//	genfeatures
//
//		Generates an HDF5 file containing the features and metadata for the slides in the
//		specified directory.
//
//	Build with:
//
//		g++ -std=c++11 -o genfeat genfeatures.cpp -lhdf5 -lhdf5_hl
//
//
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <glob.h>
#include <thread>
#include <mutex>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <ctime>

#include "hdf5.h"
#include "hdf5_hl.h"



using namespace std;



struct SlideData {

	SlideData() {
		slide.clear();
		cent_x = NULL;
		cent_y = NULL;
		features = NULL;
		mean = NULL;
		stdDev = NULL;
		numObjs = 0;
		bufferObjs = 0;
	}

	void Clear(void) {
		slide.clear();
		if( cent_x ) {
			free(cent_x);
			cent_x = NULL;
		}

		if( cent_y ) {
			free(cent_y);
			cent_y = NULL;
		}

		if( features ) {
			free(features);
			features = NULL;
		}

		if( mean ) {
			free(mean);
			mean = NULL;
		}

		if( stdDev ) {
			free(stdDev);
			stdDev = NULL;
		}
		numObjs = 0;
		bufferObjs = 0;
	}

	void Reset(void) {
		slide = "";
		cent_x = NULL;
		cent_y = NULL;
		features = NULL;
		mean = NULL;
		stdDev = NULL;
		numObjs = 0;
		bufferObjs = 0;
	}
	string	slide;
	float	*cent_x;
	float	*cent_y;
	float	*features;
	float	*mean;
	float	*stdDev;
	int		numObjs;
	int		bufferObjs;		// Size of buffers in objects
};







mutex	gDisplayMtx;


#define DIMS 48

#define VERSION_MAJOR	0
#define VERSION_MINOR	3



bool UpdateBuffers(SlideData& slide)
{
	bool 	result = true;
	int		newSize = 1000;
	float	*floatBuff = NULL;

	if( slide.bufferObjs != 0 ) {
		newSize = slide.bufferObjs * 2;
	} else {
		// Only need to allocate these buffers once
		//
		slide.mean = (float*)calloc(DIMS, sizeof(float));
		slide.stdDev = (float*)calloc(DIMS, sizeof(float));
	}

	floatBuff = (float*)realloc(slide.cent_x, newSize * sizeof(float));
	if( floatBuff != NULL ) {
		slide.cent_x = floatBuff;
	} else {
		result = false;
	}

	if( result ) {
		floatBuff = (float*)realloc(slide.cent_y, newSize * sizeof(float));
		if( floatBuff != NULL ) {
			slide.cent_y = floatBuff;
		} else {
			result = false;
		}
	}

	if( result ) {
		floatBuff = (float*)realloc(slide.features, newSize * DIMS * sizeof(float));
		if( floatBuff != NULL ) {
			slide.features = floatBuff;
		} else {
			result = false;
		}
	}

	if( result ) {
		slide.bufferObjs = newSize;
	}
	return result;
}





void SumObjects(SlideData& slide)
{

	// mean and stdDev buffers where cleared at allocation time. We can
	//	just add...
	//
	for(int obj = 0; obj < slide.numObjs; obj++) {
		for(int dim = 0; dim < DIMS; dim++) {
			slide.mean[dim] += slide.features[(obj * DIMS) + dim];
		}
	}
}





bool ParseTile(ifstream& inFile, /*(string data,*/ SlideData& slide)
{
	bool 	result = true, discard = false;
	int		valBegin, valEnd;
	string	object, value;
	float 	scale;

	// First line is the header, discard
	getline(inFile, object);

	while( getline(inFile, object) ) {

		valBegin = 0;
		// Features start at item 5 (starting at 0), x & y centroids are items
		// 1 and 2 (starting at 0) and image magnification is item 3
		//
		for(int i = 0; i < 5; i++) {
			valEnd = object.find("\t", valBegin + 1);
			if( i == 1 ) {
				value = object.substr(valBegin, valEnd - valBegin);
				slide.cent_x[slide.numObjs] = stof(value);
			} else if( i == 2 ) {
				value = object.substr(valBegin, valEnd - valBegin);
				slide.cent_y[slide.numObjs] = stof(value);
			} else if( i == 3 ) {
				value = object.substr(valBegin, valEnd - valBegin);
				scale = stof(value);
			}
			valBegin = valEnd + 1;
		}


		if( scale == 40.0 ) {
			slide.cent_x[slide.numObjs] *= 2;
			slide.cent_y[slide.numObjs] *= 2;
		}

		valBegin = valEnd + 1;
		for(int dim = 0; dim < DIMS; dim++) {
			valEnd = object.find("\t", valBegin);
			value = object.substr(valBegin, valEnd - valBegin);

			if( value.compare("-nan") == 0  || value.compare("nan") == 0 || value.compare("inf") == 0 ) {
				discard = true;
				break;
			}
			slide.features[(slide.numObjs * DIMS) + dim] = stof(value);
			valBegin = valEnd + 1;
		}

		if( discard ) {
			discard = false;
			continue;
		}

		slide.numObjs++;
		// Expand buffer if needed
		//
		if( slide.numObjs > (slide.bufferObjs - 5) ) {
			UpdateBuffers(slide);
		}
	}
	return result;
}







void ProcessTiles(vector<std::string>& tileList, SlideData& slide, string& slideDir)
{
	int		pos = slideDir.find_last_of("/");
	string 	slideName = slideDir.substr(pos + 1), data;
	ifstream 	inFile;

	slide.slide = slideName;
	if( !UpdateBuffers(slide) ) {
		gDisplayMtx.lock();
		cerr << "Unable to allocate initial buffers for slide " << slideName << endl;
		gDisplayMtx.unlock();
	}

	for(int tile = 0; tile < tileList.size(); tile++) {

		inFile.open(tileList[tile], ios::in);
		if( inFile.is_open() ) {

			if( !ParseTile(inFile, slide) ) {
				gDisplayMtx.lock();
				cerr << "Error processing tile: " << tileList[tile] << endl;
				gDisplayMtx.unlock();
				break;
			}
			inFile.close();
		}
	}
}







void SlideWorker(vector<string>& dirs, int offset, int numSlides, vector<SlideData>& slides)
{
	bool		tiles = true;
	SlideData  	slide;
	glob_t		globBuff;
	vector<std::string>	tileList;

	for(int i = offset; i < (offset + numSlides); i++) {

		string path = dirs[i];
		path += "/*.txt";
		glob(path.c_str(), GLOB_TILDE, NULL, &globBuff);

		if( globBuff.gl_pathc == 0 ) {
			gDisplayMtx.lock();
			cerr << "No tiles found in " << dirs[i] << endl;
			gDisplayMtx.unlock();
			tiles = false;
		}

		if( tiles ) {

			slide.slide = dirs[i];
			for(int tile = 0; tile < globBuff.gl_pathc; tile++) {
				tileList.push_back(globBuff.gl_pathv[tile]);
			}
			ProcessTiles(tileList, slide, dirs[i]);
			tileList.clear();
		}

		SumObjects(slide);
		slides[i] = slide;
		slide.Reset();
	}

}





int Normalize(vector<SlideData>& slides)
{
	int		totalObjs = slides[0].numObjs;
	vector<SlideData>::iterator	it;

	// Sum all slides
	for(int slide = 1; slide < slides.size(); slide++) {
		totalObjs += slides[slide].numObjs;
		for(int dim = 0; dim < DIMS; dim++) {
			slides[0].mean[dim] += slides[slide].mean[dim];
		}
	}

	// Calc mean
	for(int dim = 0; dim < DIMS; dim++) {
		slides[0].mean[dim] /= totalObjs;
	}

	// Calc std dev
	float temp;
	for(it = slides.begin(); it != slides.end(); it++) {
		for(int obj = 0; obj < it->numObjs; obj++) {
			for(int dim = 0; dim < DIMS; dim++) {
				temp = it->features[(obj * DIMS) + dim] - slides[0].mean[dim];
				temp *= temp;
				slides[0].stdDev[dim] += temp;
			}
		}
	}
	for(int dim = 0; dim < DIMS; dim++) {
		slides[0].stdDev[dim] = sqrt(slides[0].stdDev[dim] / (totalObjs - 1));
	}

	// Normalize each object
	for(it = slides.begin(); it != slides.end(); it++) {
		for(int obj = 0; obj < it->numObjs; obj++) {
			for(int dim = 0; dim < DIMS; dim++) {
				temp = it->features[(obj * DIMS) + dim];
				temp = ((temp - slides[0].mean[dim]) / slides[0].stdDev[dim]);
				it->features[(obj * DIMS) + dim] = temp;
			}
		}
	}
	return totalObjs;
}






int SaveDataspace(hid_t fileId, string dataSpace, hsize_t dim, size_t type,
					vector<SlideData>& slides, size_t offset)
{
	int		result = 0, objOffset;
	hid_t	datasetId, fileSpaceId, pList, memSpaceId;
	hsize_t	dims[2] = {0, dim}, maxDims[2] = {H5S_UNLIMITED, dim},
			chunkDims[2] = {100, dim}, start[2] = {0, 0}, size[2] = {0, dim};
	herr_t	status;
	vector<SlideData>::iterator it;
	char	*base;
	void 	**ptr;
	SlideData	slide;

	// Create features dataspace
	//
	if( result == 0 ) {
		fileSpaceId = H5Screate_simple(2, dims, maxDims);
		if( fileSpaceId < 0 ) {
			cerr << "Unable to create fileSpace" << endl;
			result = -11;
		}
	}

	if( result == 0 ) {
		pList = H5Pcreate(H5P_DATASET_CREATE);
		H5Pset_layout(pList, H5D_CHUNKED);
		H5Pset_chunk(pList, 2, chunkDims);
		datasetId = H5Dcreate(fileId, dataSpace.c_str(), type, fileSpaceId, H5P_DEFAULT, pList, H5P_DEFAULT);
		if( datasetId < 0 ) {
			cerr << "Unable to create features dataset" << endl;
			result = -12;
		}

		H5Pclose(pList);
		H5Sclose(fileSpaceId);
	}

	objOffset = 0;

	// Write features
	if( result == 0 ) {

		// Create space and adjust it for each slide
		memSpaceId = H5Screate_simple(2, dims, NULL);

		for(it = slides.begin(); it != slides.end(); it++) {

			// Resize memory space
			dims[0] = it->numObjs;
			H5Sset_extent_simple(memSpaceId, 2, dims, NULL);

			// Extend dataset
			dims[0] = objOffset + it->numObjs;
			H5Dset_extent(datasetId, dims);

			// Get hyperslab to write to
			size[0] = it->numObjs;
			start[0] = objOffset;
			fileSpaceId = H5Dget_space(datasetId);
			H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, start, NULL, size, NULL);

			slide = *it;
			base = (char*)(&slide);
			ptr = (void**)(base + offset);

			H5Dwrite(datasetId, type, memSpaceId, fileSpaceId, H5P_DEFAULT, *ptr);
			H5Sclose(fileSpaceId);

			objOffset += it->numObjs;
		}
		H5Sclose(memSpaceId);
		H5Dclose(datasetId);
	}
	return result;
}







int SaveSlideIdx(hid_t fileId, vector<SlideData>& slides, int totalObjs)
{
	int		result = 0, objOffset = 0,
 			*slideIdx = (int*)malloc(totalObjs * sizeof(int)),
 			*dataIdx = (int*)malloc(slides.size() * sizeof(int));
 	hsize_t	dims[2];
	herr_t	status;

	if( slideIdx == NULL || dataIdx == NULL ) {
		cerr << "Unable to allocate slide index" << endl;
		result = -20;
	}

	if( result == 0 ) {
		for(int slide = 0; slide < slides.size(); slide++) {
			dataIdx[slide] = objOffset;
			for(int obj = 0; obj < slides[slide].numObjs; obj++) {
				slideIdx[objOffset++] = slide;
			}
		}

		dims[0] = totalObjs;
		dims[1] = 1;
		status = H5LTmake_dataset(fileId, "/slideIdx", 2, dims, H5T_NATIVE_INT, slideIdx);
		if( status < 0 ) {
			cerr << "Unable to save slide index" << endl;
			result = -21;
		}

		dims[0] = slides.size();
		status = H5LTmake_dataset(fileId, "/dataIdx", 2, dims, H5T_NATIVE_INT, dataIdx);
		if( status < 0 ) {
			cerr << "Unable to save slide index" << endl;
			result = -22;
		}
	}

	if( slideIdx )
		free(slideIdx);
	if( dataIdx )
		free(dataIdx);
	return result;
}




int SaveNormalization(hid_t fileId, SlideData& slide)
{
	int		result = 0;
	hsize_t	dims[2];
	herr_t	status;

	dims[0] = DIMS;
	dims[1] = 1;

	status = H5LTmake_dataset(fileId, "/mean", 2, dims, H5T_NATIVE_FLOAT, slide.mean);
	if( status < 0 ) {
		cerr << "Unable to save mean data" << endl;
		result = -30;
	}

	status = H5LTmake_dataset(fileId, "/std_dev", 2, dims, H5T_NATIVE_FLOAT, slide.stdDev);
	if( status < 0 ) {
		cerr << "Unable to save mean data" << endl;
		result = -31;
	}
	return result;
}







int SaveSlides(hid_t fileId, vector<SlideData>& slides)
{
	int		result = 0, nameLen;
	char	**slideNames = NULL;

	slideNames = (char**)malloc(slides.size() * sizeof(char*));
	if( slideNames == NULL ) {
		cerr << "Unable to allocate slide buffer" << endl;
		result = -40;
	}

	if( result == 0 ) {
		for(int i = 0; i < slides.size(); i++) {
			nameLen = slides[i].slide.length() + 1;
			slideNames[i] = (char*)malloc(nameLen);
			if( slideNames[i] ) {
				strncpy(slideNames[i], slides[i].slide.data(), nameLen);
			} else {
				result = -41;
				break;
			}
		}
	}

	if( result == 0 ) {
		hsize_t size = slides.size();
		hid_t	dSpace = H5Screate_simple(1, &size, NULL),
				dType, dataSet;
		if( dSpace < 0 ) {
			result = -42;
		} else {
			dType = H5Tcopy(H5T_C_S1);
			H5Tset_size(dType, H5T_VARIABLE);
			dataSet = H5Dcreate(fileId, "slides", dType, dSpace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
			if( dataSet < 0 ) {
				result = -43;
			} else {
				H5Dwrite(dataSet, dType, dSpace, H5S_ALL, H5P_DEFAULT, slideNames);
				H5Dclose(dataSet);
				H5Sclose(dSpace);
				H5Tclose(dType);
			}
		}
	}

	return result;
}



int SaveProvenance(hid_t fileId, string cmdLine)
{
	int		result = 0;
	hsize_t	dims[2];
	herr_t	status;

	struct utsname	hostInfo;
	if( uname(&hostInfo) ) {
		result = -50;
	}

	if( result == 0 ) {
		string sysInfo = hostInfo.nodename;
		sysInfo += ", ";
		sysInfo += hostInfo.sysname;
		sysInfo += " (";
		sysInfo += hostInfo.release;
		sysInfo += " ";
		sysInfo += hostInfo.machine;
		sysInfo += ")";

		status = H5LTset_attribute_string(fileId, "/", "host info", sysInfo.c_str());
		if( status < 0 ) {
			result = -51;
		}
	}

	if( result == 0 ) {
		int ver[2] = {VERSION_MAJOR, VERSION_MINOR};

		status = H5LTset_attribute_int(fileId, "/", "version", ver, 2);
		if( status < 0 ) {
			result = -52;
		}
	}

	if( result == 0) {
		time_t t = time(0);
		struct tm *now = localtime(&t);
		char curTime[100];

		sprintf(curTime, "%d-%d-%4d, %02d:%02d",
	    		now->tm_mon + 1, now->tm_mday, now->tm_year + 1900,
	    		now->tm_hour, now->tm_min);

		status = H5LTset_attribute_string(fileId, "/", "creation date", curTime);
		if( status < 0 ) {
			result = -53;
		}
	}

	if( result == 0) {
		status = H5LTset_attribute_string(fileId, "/", "command line", cmdLine.c_str());
		if( status < 0 ) {
			cerr << "Saving source file failed" << endl;
			result = -54;
		}
	}
	return result;
}





int Save(vector<SlideData>& slides, string fileName, int totalObjs, string cmdLine)
{
	int		result = 0;
	hid_t	fileId;


	fileId = H5Fcreate(fileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	if( fileId < 0 ) {
		cerr << "Unable to create " << fileName << endl;
		result = -10;
	}

	if( result == 0 ) {
		result = SaveDataspace(fileId, "features", DIMS, H5T_NATIVE_FLOAT, slides, offsetof(SlideData, features));
	}
	if( result == 0 ) {
		result = SaveDataspace(fileId, "x_centroid", 1, H5T_NATIVE_FLOAT, slides, offsetof(SlideData, cent_x));
	}
	if( result == 0 ) {
		result = SaveDataspace(fileId, "y_centroid", 1, H5T_NATIVE_FLOAT, slides, offsetof(SlideData, cent_y));
	}
	if( result == 0 ) {
		result = SaveSlideIdx(fileId, slides, totalObjs);
	}
	if( result == 0 ) {
		result = SaveNormalization(fileId, slides[0]);
	}
	if( result == 0 ) {
		result = SaveSlides(fileId, slides);
	}
	if( result == 0 ) {
		result = SaveProvenance(fileId, cmdLine);
	}
	H5Fclose(fileId);
	return result;
}







int	ProcessSlides(vector<std::string>& dirs, string outFile, string cmdLine)
{
	int 	result = 0, totalObjs;
	vector<SlideData>	slides;
	vector<std::thread>	workers;
	int		numThreads = thread::hardware_concurrency(), offset, slideCnt, remain, slidesPer;

	cout << "Processing slides..." << endl;
	slides.resize(dirs.size());
	remain = dirs.size() % numThreads;
	slidesPer = dirs.size() / numThreads;

	for(int i = 0; i < numThreads - 1; i++) {
		slideCnt = slidesPer + ((i < remain) ? 1 : 0);
		offset = (i < remain) ? i * (slidesPer + 1) :
				(remain * (slidesPer + 1)) + ((i - remain) * slidesPer);

		workers.push_back(std::thread(SlideWorker, std::ref(dirs), offset,
						  slideCnt, std::ref(slides)));
	}

	// Main thread's workload
	offset = (remain * (slidesPer + 1)) + ((numThreads - remain - 1) * slidesPer);
	SlideWorker(dirs, offset, slidesPer, slides);

	for(auto &t: workers) {
		t.join();
	}

	// Normalize and write to HDF5 file
	//
	cout << "Normalizing..." << endl;
	totalObjs = Normalize(slides);
	cout << "Saving..." << endl;
	result = Save(slides, outFile, totalObjs, cmdLine);
	cout << "Done!" << endl;
	return result;
}






int main(int argc, char *argv[])
{
	int result = 0;
	glob_t	globBuff;
	vector<std::string>	slideDirs;


	if( argc != 3 ) {
		cerr << "Usage: " << argv[0] << " <slide data dir> <outfile>" << endl;
		result = -1;
	}

	if( result == 0 ) {
		// Get a list of the slide directories
		string path = argv[1];
		path += "/*";

		glob(path.c_str(), GLOB_TILDE, NULL, &globBuff);
		if( globBuff.gl_pathc == 0 ) {
			cerr << "Slide data not found in " << argv[1] << endl;
			result = -2;
		} else {
			for(int i = 0; i < globBuff.gl_pathc; i++) {
				slideDirs.push_back(globBuff.gl_pathv[i]);
			}
		}
	}

	string cmdLine;
	for(int i = 0; i < argc; i++) {
		cmdLine += argv[i];
		cmdLine += " ";
	}

	if( result == 0 ) {
		result = ProcessSlides(slideDirs, argv[2], cmdLine);
	}


	return result;
}
