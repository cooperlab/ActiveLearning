//
//	Copyright (c) 2014-2016, Emory University
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
#include <cstdlib>
#include <unistd.h>
#include <jansson.h>
#include <sys/stat.h>

#include "picker.h"
#include "commands.h"
#include "logger.h"




using namespace std;



const char passResp[] = "PASS";
const char failResp[] = "FAIL";



extern EvtLogger *gLogger;


Picker::Picker(string dataPath, string outPath, string heatmapPath) :
m_dataset(NULL),
m_classTrain(NULL),
m_labels(NULL),
m_ids(NULL),
m_trainSet(NULL),
m_dataPath(dataPath),
m_outPath(outPath),
m_slideIdx(NULL),
m_xCentroid(NULL),
m_yCentroid(NULL),
m_xClick(NULL),
m_yClick(NULL)
{
	memset(m_UID, 0, UID_LENGTH + 1);
	m_samples.clear();
}





Picker::~Picker(void)
{
	Cleanup();
}





//-----------------------------------------------------------------------------




void Picker::Cleanup(void)
{

	m_samples.clear();
	m_classNames.clear();

	if( m_dataset ) {
		delete m_dataset;
		m_dataset = NULL;
	}

	if( m_labels ) {
		free(m_labels);
		m_labels = NULL;
	}

	if( m_ids ) {
		free(m_ids);
		m_ids = NULL;
	}

	if( m_trainSet ) {
		free(m_trainSet);
		m_trainSet = NULL;
	}

	if( m_xCentroid ) {
		free(m_xCentroid);
		m_xCentroid = NULL;
	}

	if( m_yCentroid ) {
		free(m_yCentroid);
		m_yCentroid = NULL;
	}

	if( m_xClick ) {
		free(m_xClick);
		m_xClick = NULL;
	}

	if( m_yClick ) {
		free(m_yClick);
		m_yClick = NULL;
	}
}






bool Picker::ParseCommand(const int sock, const char *data, int size)
{
	bool		result = true;
	json_t		*root;
	json_error_t error;

	root = json_loads(data, 0, &error);
	if( !root ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Error parsing json");
		result = false;
	}

	if( result ) {
		json_t	*cmdObj = json_object_get(root, "command");

		if( !json_is_string(cmdObj) ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Command is not a string");
			result = false;
		} else {
			const char	*command = json_string_value(cmdObj);

			gLogger->LogMsg(EvtLogger::Evt_INFO, "Processing: %s", command);

			// 	Be careful with command names that can be a prefix of another.
			// 	i.e. init can be a prefix of initPicker. If you want to do this
			//	check for the prefix only version ("init" in the previous example)
			//	before the others.
			//
			if( strncmp(command, CMD_PICKINIT, strlen(CMD_PICKINIT)) == 0) {
				result = InitPicker(sock, root);
			} else if( strncmp(command, CMD_PICKADD, strlen(CMD_PICKADD)) == 0) {
				result = AddObjects(sock, root);
			} else if( strncmp(command, CMD_PICKCNT, strlen(CMD_PICKCNT)) == 0) {
				result = PickerStatus(sock, root);
			} else if( strncmp(command, CMD_END, strlen(CMD_END)) == 0 ) {
				result = CancelSession(sock, root);
			} else if( strncmp(command, CMD_PICKEND, strlen(CMD_PICKEND)) == 0) {
				result = PickerFinalize(sock, root);
			} else if( strncmp(command, CMD_APPLY, strlen(CMD_APPLY)) == 0 ) {
				result = GetSelected(sock, root);
			} else {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Invalid command");
				result = false;
			}
		}
		json_decref(root);
	}
	return result;
}











bool Picker::UpdateBuffers(int updateSize, bool includeClick)
{
	bool	result = true;
	int		*newBuff = NULL, newSize = m_samples.size() + updateSize;
	float	*floatBuff = NULL;

	newBuff = (int*)realloc(m_labels, newSize * sizeof(int));
	if( newBuff != NULL )
		m_labels = newBuff;
	else
		result = false;

	if( result ) {
		newBuff = (int*)realloc(m_ids, newSize * sizeof(int));
		if( newBuff != NULL )
			m_ids = newBuff;
		else
			result = false;
	}

	if( result ) {
		floatBuff = (float*)realloc(m_xCentroid, newSize * sizeof(float));
		if( floatBuff != NULL )
			m_xCentroid = floatBuff;
		else
			result = false;
	}

	if( result ) {
		floatBuff = (float*)realloc(m_yCentroid, newSize * sizeof(float));
		if( floatBuff != NULL )
			m_yCentroid = floatBuff;
		else
			result = false;
	}

	if( result ) {
		newBuff = (int*)realloc(m_slideIdx, newSize * sizeof(int));
		if( newBuff != NULL )
			m_slideIdx = newBuff;
		else
			result = false;
	}

	if( includeClick && result ) {
		floatBuff = (float*)realloc(m_xClick, newSize * sizeof(float));
		if( floatBuff != NULL )
			m_xClick = floatBuff;
		else
			result = false;

		if( result ) {
			floatBuff = (float*)realloc(m_yClick, newSize * sizeof(float));
			if( floatBuff != NULL )
				m_yClick = floatBuff;
			else
				result = false;
		}
	}

	if( result ) {
		bool  init = (m_trainSet == NULL) ? true : false;
		float **newFeatureIdx = (float**)realloc(m_trainSet, newSize * sizeof(float*));
		if( newFeatureIdx != NULL ) {
			m_trainSet = newFeatureIdx;
			if( init ) {
				m_trainSet[0] = NULL;
			}
		} else {
			result = false;
		}
	}

	if( result ) {
		int 	dims = m_dataset->GetDims();
		float *newFeatBuff = (float*)realloc(m_trainSet[0], newSize * dims * sizeof(float));
		if( newFeatBuff != NULL ) {
			m_trainSet[0] = newFeatBuff;
			for(int i = 1; i < newSize; i++) {
				m_trainSet[i] = m_trainSet[i - 1] + dims;
			}
		} else
			result = false;
	}
	return result;
}





bool Picker::CancelSession(const int sock, json_t *obj)
{
	bool	result = true;

	// Just erase the UID for now
	memset(m_UID, 0, UID_LENGTH + 1);
	gLogger->LogMsg(EvtLogger::Evt_INFO, "Session canceled");

	Cleanup();

	size_t bytesWritten = ::write(sock, (result) ? passResp : failResp ,
							((result) ? sizeof(passResp) : sizeof(failResp)) - 1);
	if( bytesWritten != sizeof(failResp) - 1 )
		result = false;

	return result;
}






bool Picker::SaveTrainingSet(string fileName)
{
	bool	result = false;;
	MData 	*trainingSet = new MData();

	if( trainingSet != NULL ) {

		result = trainingSet->Create(m_trainSet[0], m_samples.size(), m_dataset->GetDims(),
							m_labels, m_ids, NULL, m_dataset->GetMeans(), m_dataset->GetStdDevs(),
							m_xCentroid, m_yCentroid, m_dataset->GetSlideNames(), m_slideIdx,
							m_dataset->GetNumSlides(), m_classNames);
	}

	if( result ) {
		fileName = m_outPath + fileName;
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Saving training set to: %s", fileName.c_str());
		result = trainingSet->SaveAs(fileName);
	}

	if( trainingSet != NULL )
		delete trainingSet;


	return result;
}








bool Picker::GetSelected(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*value;
	const char *slideName = NULL;
	int		xMin, xMax, yMin, yMax;

	value = json_object_get(obj, "slide");
	slideName = json_string_value(value);
	value = json_object_get(obj, "xMin");
	xMin = json_integer_value(value);
	value = json_object_get(obj, "xMax");
	xMax = json_integer_value(value);
	value = json_object_get(obj, "yMin");
	yMin = json_integer_value(value);
	value = json_object_get(obj, "yMax");
	yMax = json_integer_value(value);

	if( slideName == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplyClassifier) Unable to decode slide name");
		result = false;
	}
	if( xMin == 0 || xMax == 0 || yMin == 0 || yMax == 0 ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplyClassifier) Unable to decode min/max values");
		result = false;
	}

	double timing = gLogger->WallTime();
	if( result ) {
		value = json_object_get(obj, "uid");
		const char *uid = json_string_value(value);
		result = IsUIDValid(uid);
 	}

	if( result ) {
		float 	**ptr;
		int 	*labels = NULL, dims, slideObjs, offset;

		offset = m_dataset->GetSlideOffset(slideName, slideObjs);
		labels = (int*)malloc(slideObjs * sizeof(int));

		if( labels == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(GetSelected) Unable to allocate labels buffer");
			result = false;
		}

		if( result ) {

			// In picker mode, we just set selected objects to the positive
			// class to indicate they've been selected already. All other
			// objects are set to the negative class.
			//
			memset(labels, -1, slideObjs * sizeof(int));

			vector<int>::iterator	it;
			for(it = m_samples.begin(); it != m_samples.end(); it++) {
				// Only mark samples that are on this slide
				if( *it >= offset && *it < (offset + slideObjs) ) {
					labels[*it - offset] = 1;
				}
			}
		}

		if( result ) {
			result = SendSelected(xMin, xMax, yMin, yMax, slideName, labels, sock);
		}

		if( labels )
			free(labels);
	}
	timing = gLogger->WallTime() - timing;
	gLogger->LogMsg(EvtLogger::Evt_INFO, "Classification took: %f", timing);

	return result;
}





bool Picker::SendSelected(int xMin, int xMax, int yMin, int yMax,
								 string slide, int *results, const int sock)
{
	bool	result = true;
	json_t	*root = json_object();

	if( root == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(SendSelected) Unable to create JSON object");
		result = false;
	}

	if( result ) {
		char	tag[25];
		int		offset, slideObjs;

		offset = m_dataset->GetSlideOffset(slide, slideObjs);
		for(int i = offset; i < offset + slideObjs; i++) {

			if( slide.compare(m_dataset->GetSlide(i)) == 0 &&
				m_dataset->GetXCentroid(i) >= xMin && m_dataset->GetXCentroid(i) <= xMax &&
				m_dataset->GetYCentroid(i) >= yMin && m_dataset->GetYCentroid(i) <= yMax ) {

				snprintf(tag, 24, "%.1f_%.1f",
						m_dataset->GetXCentroid(i), m_dataset->GetYCentroid(i));

				json_object_set(root, tag, json_integer((results[i - offset] == -1) ? 0 : 1 ));
			}
		}

		if( result ) {
			char *jsonObj = json_dumps(root, 0);

			if( jsonObj == NULL ) {
				result = false;
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(SendSelected) Unable to encode JSON");
			}

			if( result ) {
				size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

				if( bytesWritten != strlen(jsonObj) ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "(SendSelected) Unable to send entire JSON object");
					result = false;
				}
			}
			json_decref(root);
			free(jsonObj);
		}
	}
	return result;
}





bool Picker::LoadDataset(string dataSetFileName)
{
	bool result = true;

	// Load dataset only if not loaded already
	//
	if( m_curDatasetName.compare(dataSetFileName) != 0 ) {
		m_curDatasetName = dataSetFileName;

		if( m_dataset )
			delete m_dataset;

		m_dataset = new MData();
		if( m_dataset == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(LoadDataset) Unable to create dataset object");
			result = false;
		}

		if( result ) {
			string fqn = m_dataPath + dataSetFileName;
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Loading: %s", fqn.c_str());
			result = m_dataset->Load(fqn);
		}
	}
	return result;
}






bool Picker::InitPicker(const int sock, json_t *obj)
{
	bool	result = true, uidUpdated = false;
	json_t	*jsonObj;
	const char *fileName, *uid, *testSetName;

	// m_UID's length is 1 greater than UID_LENGTH, So we can
	// always write a 0 there to make strlen safe.
	//
	m_UID[UID_LENGTH] = 0;

	if( strlen(m_UID) > 0 ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Session already in progress: %s", m_UID);
 		result = false;
	}

	if( result ) {
		jsonObj = json_object_get(obj, "features");
		fileName = json_string_value(jsonObj);
		if( fileName == NULL ) {
			result = false;
		}
	}

	if( result ) {
		jsonObj = json_object_get(obj, "uid");
		uid = json_string_value(jsonObj);
		if( uid == NULL ) {
			result = false;
		}
	}

	if( result ) {
		strncpy(m_UID, uid, UID_LENGTH);
		uidUpdated = true;
		m_dataset = new MData();
		if( m_dataset == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create dataset object");
			result = false;
		}
	}

	if( result ) {
		jsonObj = json_object_get(obj, "name");
		testSetName = json_string_value(jsonObj);
		if( testSetName != NULL ) {
			m_testsetName = testSetName;
		} else {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to extract test set name");
			result = false;
		}
	}

	if( result ) {
		jsonObj = json_object_get(obj, "negClass");
		testSetName = json_string_value(jsonObj);
		if( testSetName != NULL ) {
			m_classNames.push_back(testSetName);
		} else {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to extract negative class name");
			result = false;
		}
	}

	if( result ) {
		jsonObj = json_object_get(obj, "posClass");
		testSetName = json_string_value(jsonObj);
		if( testSetName != NULL ) {
			m_classNames.push_back(testSetName);
		} else {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to extract positive class name");
			result = false;
		}
	}


	if( result ) {
		string fqFileName = m_dataPath + string(fileName);

		gLogger->LogMsg(EvtLogger::Evt_INFO, "Loading %s", fqFileName.c_str());
		double	start = gLogger->WallTime();
		result = m_dataset->Load(fqFileName);
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Loading took %f", gLogger->WallTime() - start);
	}


	// Send result back to client
	//
	size_t bytesWritten = ::write(sock, (result) ? passResp : failResp ,
								((result) ? sizeof(passResp) : sizeof(failResp)) - 1);

	if( bytesWritten != sizeof(failResp) - 1 )
		result = false;

	if( !result && uidUpdated ){
		// Initialization failed, clear current UID
		memset(m_UID, 0, UID_LENGTH + 1);
	}


	return result;
}





bool Picker::AddObjects(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*sampleArray, *value, *jsonObj;

	// Check for valid UID
	//
	value = json_object_get(obj, "uid");
	const char *uid = json_string_value(value);

	result = IsUIDValid(uid);

	if( result ) {
		sampleArray = json_object_get(obj, "samples");
		if( !json_is_array(sampleArray) ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Invalid samples array");
			result = false;
		}
	}

	if( result ) {
		// Make room for the new samples
		result = UpdateBuffers(json_array_size(sampleArray), true);
	}

	if( result ) {
		size_t	index;
		int		id, label, idx, dims = m_dataset->GetDims();
		float	centX, centY, clickX, clickY;
		const char *slide;

		json_array_foreach(sampleArray, index, jsonObj) {
			value = json_object_get(jsonObj, "id");
			id = json_integer_value(value);

			// The dynamic typing in PHP or javascript can make a float
			// an int if it has no decimal portion. Since the centroids can
			// be whole numbers, we need to check if they are real, if not
			// we just assume they are integer
			//
			value = json_object_get(jsonObj, "centX");
			if( json_is_real(value) )
				centX = (float)json_real_value(value);
			else
				centX = (float)json_integer_value(value);

			value = json_object_get(jsonObj, "centY");
			if( json_is_real(value) )
				centY = (float)json_real_value(value);
			else
				centY = (float)json_integer_value(value);

			value = json_object_get(jsonObj, "clickX");
			if( json_is_real(value) )
				clickX = (float)json_real_value(value);
			else
				clickX = (float)json_integer_value(value);

			value = json_object_get(jsonObj, "clickY");
			if( json_is_real(value) )
				clickY = (float)json_real_value(value);
			else
				clickY = (float)json_integer_value(value);

			value = json_object_get(jsonObj, "label");
			label = json_integer_value(value);

			//
			// FIXME!!! - Need to handle slide names that are numbers better
			//
			value = json_object_get(jsonObj, "slide");
			if( json_is_string(value) )
				slide = json_string_value(value);
			else {
				char name[255];
				int num = json_integer_value(value);
				snprintf(name, 255, "%d", num);
				slide = name;
			}

			// Get the dataset index for this object
			idx = m_dataset->FindItem(centX, centY, slide);
 			int	pos = m_samples.size();

			if( idx != -1 ) {
				m_labels[pos] = label;
				m_ids[pos] = id;
//				m_sampleIter[pos] = m_iteration;
				m_slideIdx[pos] = m_dataset->GetSlideIdx(slide);
				m_xCentroid[pos] = m_dataset->GetXCentroid(idx);
				m_yCentroid[pos] = m_dataset->GetYCentroid(idx);
				m_xClick[pos] = clickX;
				m_yClick[pos] = clickY;
				result = m_dataset->GetSample(idx, m_trainSet[pos]);
				m_samples.push_back(idx);
			} else {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to find item: %s, %f, %f ", slide, centX, centY);
				result = false;
			}

			// Something is wrong, stop processing
			if( !result )
				break;
		}
	}

	json_t *root = NULL;
	if( result ) {
		root = json_object();

		if( root == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR,  "(AddObjects) Error creating JSON object");
			result = false;
		}
	}

	gLogger->LogMsg(EvtLogger::Evt_ERROR, "Encoding response");
	if( result ) {

		json_object_set(root, "count", json_integer(m_samples.size()));
		json_object_set(root, "status", json_string((result) ? "PASS" : "FAIL"));

		// Send result back to client
		//
		char *jsonObj = json_dumps(root, 0);
		size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;

		json_decref(root);
		free(jsonObj);
	}

	gLogger->LogMsg(EvtLogger::Evt_ERROR, "Done, result: %d", result);
	return result;
}






bool Picker::PickerStatus(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*value, *root = NULL;

	// Check for valid UID
	//
	value = json_object_get(obj, "uid");
	const char *uid = json_string_value(value);

	result = IsUIDValid(uid);

	if( result ) {
		root = json_object();
		if( root == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR,  "(PickerStatus) Error creating JSON object");
			result = false;
		}
	}

	if( result ) {
		json_object_set(root, "count", json_integer(m_samples.size()));
		json_object_set(root, "status", json_string((result) ? "PASS" : "FAIL"));

		// Send result back to client
		//
		char *jsonObj = json_dumps(root, 0);
		size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;

		json_decref(root);
		free(jsonObj);
	}
	return true;
}







bool Picker::PickerFinalize(const int sock, json_t *obj)
{
	bool	result = false;
	MData 	*testSet = new MData();
	string 	fileName = m_testsetName + ".h5", fqfn;

	if( testSet != NULL ) {

		fqfn = m_outPath + fileName;
		struct stat buffer;
		if( stat(fqfn.c_str(), &buffer) == 0 ) {
			string 	tag = &m_UID[UID_LENGTH - 3];

			fileName = m_testsetName + "_" + tag + ".h5";
			fqfn = m_outPath + fileName;
		}

		result = testSet->Create(m_trainSet[0], m_samples.size(), m_dataset->GetDims(),
							m_labels, m_ids, NULL, m_dataset->GetMeans(), m_dataset->GetStdDevs(),
							m_xCentroid, m_yCentroid, m_dataset->GetSlideNames(), m_slideIdx,
							m_dataset->GetNumSlides(), m_classNames, m_xClick, m_yClick);
	}

	if( result ) {
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Saving test set to: %s", fqfn.c_str());
		result = testSet->SaveAs(fqfn);
	}

	if( testSet != NULL )
		delete testSet;

	json_t	*root;

	if( result ) {
		root = json_object();
		if( root == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR,  "(PickerStatus) Error creating JSON object");
			result = false;
		}
	}

	if( result ) {
		// This session is done, clear the UID and cleanup associated
		// data
		//
		memset(m_UID, 0, UID_LENGTH + 1);
		Cleanup();
	}

	if( result ) {

		json_object_set(root, "filename", json_string(fqfn.c_str()));
		json_object_set(root, "status", json_string((result) ? "PASS" : "FAIL"));

		// Send result back to client
		//
		char *jsonObj = json_dumps(root, 0);
		size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;

		json_decref(root);
		free(jsonObj);
	}

	return result;
}


