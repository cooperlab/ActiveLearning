#include <cstdlib>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <jansson.h>
#include <sys/stat.h>
#include <algorithm>

#include "learner.h"
#include "logger.h"



using namespace std;

const char passResp[] = "PASS";
const char failResp[] = "FAIL";

#define INITIAL_OBJS 8

extern EvtLogger *gLogger;


Learner::Learner(string dataPath, string outPath) :
m_dataset(NULL),
m_classTrain(NULL),
m_labels(NULL),
m_ids(NULL),
m_trainSet(NULL),
m_classifier(NULL),
m_sampler(NULL),
m_dataPath(dataPath),
m_outPath(outPath),
m_sampleIter(NULL),
m_curAccuracy(0.0f)
{
	memset(m_UID, 0, UID_LENGTH + 1);
	m_samples.clear();

	srand(time(NULL));
}





Learner::~Learner(void)
{
	Cleanup();
}





//-----------------------------------------------------------------------------




void Learner::Cleanup(void)
{
	m_samples.clear();

	if( m_dataset ) {
		delete m_dataset;
		m_dataset = NULL;
	}

	if( m_classifier ) {
		delete m_classifier;
		m_classifier = NULL;
	}

	if( m_sampler ) {
		delete m_sampler;
		m_sampler = NULL;
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

	if( m_sampleIter ) {
		free(m_sampleIter);
		m_sampleIter = NULL;
	}
}






bool Learner::ParseCommand(const int sock, char *data, int size)
{
	bool		result = true;
	json_t		*root;
	json_error_t error;

	data[size] = 0;
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

			gLogger->LogMsg(EvtLogger::Evt_INFO, "Processing: " + string(command));

			// TODO - Get rid of hard-coded commands
			//
			if( strncmp(command, "init", 4) == 0 ) {
				result = StartSession(sock, root);
			} else if( strncmp(command, "prime", 5) == 0 ) {
				result = Submit(sock, root);
			} else if( strncmp(command, "select", 6) == 0 ) {
				result = this->Select(sock, root);
			} else if( strncmp(command, "end", 3) == 0 ) {
				result = CancelSession(sock, root);
			} else if( strncmp(command, "submit", 6) == 0 ) {
				result = Submit(sock, root);
			} else if( strncmp(command, "finalize", 8) == 0 ) {
				result = FinalizeSession(sock, root);
			} else if( strncmp(command, "apply", 8) == 0 ) {
				result = ApplyClassifier(sock, root);
			} else if( strncmp(command, "visualize", 9) == 0 ) {
				result = Visualize(sock, root);
			} else {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Invalid command");
				result = false;
			}
		}
		json_decref(root);
	}
	return result;
}





bool Learner::StartSession(const int sock, json_t *obj)
{
	bool	result = true, uidUpdated = false;
	json_t	*jsonObj;
	const char *fileName, *uid, *classifierName;

	// m_UID's length is 1 greater than UID_LENGTH, So we can
	// always write a 0 there to make strlen safe.
	//
	m_UID[UID_LENGTH] = 0;

	if( strlen(m_UID) > 0 ) {
		char msg[100];
		snprintf(msg, 100, "Session already in progress: %s", m_UID);
		gLogger->LogMsg(EvtLogger::Evt_ERROR, msg);
 		result = false;
	}

	if( result ) {
		m_iteration = 0;
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
		classifierName = json_string_value(jsonObj);
		if( classifierName != NULL ) {
			m_classifierName = classifierName;
		} else {
			result = false;
		}
	}

	if( result ) {
		string fqFileName = m_dataPath + string(fileName);

		gLogger->LogMsg(EvtLogger::Evt_INFO, "Loading " + fqFileName);

		result = m_dataset->Load(fqFileName);
	}

	// Create classifier and sampling objects
	//
	if( result ) {
		char msg[100];

		snprintf(msg, 100, "Loaded... %d objects of %d dimensions", m_dataset->GetNumObjs(), m_dataset->GetDims());
		gLogger->LogMsg(EvtLogger::Evt_INFO, msg);

 		m_classifier = new OCVBinarySVM();
		if( m_classifier == NULL ) {
			result = false;
		}
	}

	if( result ) {
		m_sampler = new UncertainSample(m_classifier, m_dataset);
		//m_sampler = new RandomSample(m_dataset);
		if( m_sampler == NULL ) {
			result = false;
		}
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




//
// Select new samples for the user to label.
//
//
bool Learner::Select(const int sock, json_t *obj)
{
	bool	result = true;
	json_t 	*root = NULL, *value = NULL, *sample = NULL, *sampleArray = NULL;
	int		reqIteration, idx;
	float	score;

	// m_UID's length is 1 greater than UID_LENGTH, So we can
	// always write a 0 there to make strlen safe.
	//
	m_UID[UID_LENGTH] = 0;

	if( strlen(m_UID) == 0 ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(select) No active session");
		result = false;
	} else {
		value = json_object_get(obj, "uid");
		const char *uid = json_string_value(value);
		if( uid == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(select) Unable to decode UID");
			result = false;
		} else if( strncmp(uid, m_UID, UID_LENGTH) != 0 ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(select) Invalid UID");
			result = false;		
		}
	}
			
	value = json_object_get(obj, "iteration");
	if( value == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(select) Unable to decode iteration");
		result = false;
	} else {
		reqIteration = json_integer_value(value);
	}
	
	if( result ) {
		root = json_object();

		if( root == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR,  "(select) Error creating JSON object");
			result = false;
		}
	}
	
	if( result ) {
		sampleArray = json_array();
		if( sampleArray == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_INFO, "(select) Unable to create sample JSON Array");
			result = false;
		}
	}

	if( result ) {

		json_object_set(root, "iteration", json_integer(m_iteration));
		json_object_set(root, "accuracy", json_real(m_curAccuracy));

		int 	*selIdx = NULL;
		float	*selScores = NULL;

		if( m_iteration != reqIteration ) {
			// Get new samples
			m_sampler->SelectBatch(8, selIdx, selScores);
		}

		for(int i = 0; i < 8; i++) {

			sample = json_object();
			if( sample == NULL ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create sample JSON object");
				result = false;
				break;
			}

			if( m_iteration != reqIteration ) {
				// Get new sample
				idx = selIdx[i];
				m_curSet.push_back(idx);
				score = selScores[i];
				m_curScores.push_back(selScores[i]);
			} else {
				// Haven't submitted that last group of selections. Send
				// them again
				score = m_curScores[i];
				idx = m_curSet[i];
			}

			json_object_set(sample, "slide", json_string(m_dataset->GetSlide(idx)));
			json_object_set(sample, "id", json_integer(0));
			json_object_set(sample, "centX", json_real(m_dataset->GetXCentroid(idx)));
			json_object_set(sample, "centY", json_real(m_dataset->GetYCentroid(idx)));
			json_object_set(sample, "label", json_integer((score < 0) ? -1 : 1));
			json_object_set(sample, "maxX", json_integer(0));
			json_object_set(sample, "maxY", json_integer(0));
			json_object_set(sample, "boundary", json_string(""));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}

		if( selIdx )
			free(selIdx);
		if( selScores )
			free(selScores);
	}

	if( result ) {
		json_object_set(root, "samples", sampleArray);
		json_decref(sampleArray);

		char *jsonObj = json_dumps(root, 0);
		size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;

		json_decref(root);
		free(jsonObj);
	}
	return result;
}






//
//	User has indicated the label for each of the previously
//	selected samples. Add them to the training set.
//
bool Learner::Submit(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*sampleArray, *value, *jsonObj;
	int		iter;

	// m_UID's length is 1 greater than UID_LENGTH, So we can
	// always write a 0 there to make strlen safe.
	//
	m_UID[UID_LENGTH] = 0;

	if( strlen(m_UID) == 0 ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(submit) No active session");
		result = false;
	} else {
		value = json_object_get(obj, "uid");
		const char *uid = json_string_value(value);
		if( uid == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(submit) Unable to decode UID");
			result = false;
		} else if( strncmp(uid, m_UID, UID_LENGTH) != 0 ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(submit) Invalid UID");
			result = false;		
		}
	}

	if( result ) {
		value = json_object_get(obj, "iteration");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(submit) Unable to decode iteration");
			result = false;
		} else {
			iter = json_integer_value(value);
			// Check iteration to make sure we're in sync
			//
			if( iter != m_iteration ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Resubmitting not allowed");;
				result = false;
			}
		}
	}
	
	if( result ) {
		sampleArray = json_object_get(obj, "samples");
		if( !json_is_array(sampleArray) ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Invalid samples array");
			result = false;
		}
	}


	if( result ) {
		// Make room for the new samples
		result = UpdateBuffers(json_array_size(sampleArray));
	}

	if( result ) {
		size_t	index;
		int		id, label, idx, dims = m_dataset->GetDims();
		float	centX, centY;
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
				sprintf(name, "%d", num);
				slide = name;
			}

			// Get the dataset index for this object
			idx = m_dataset->FindItem(centX, centY, slide);
 			int	pos = m_samples.size();

			if( idx != -1 ) {
				m_labels[pos] = label;
				m_ids[pos] = id;
				m_sampleIter[pos] = m_iteration;
				result = m_dataset->GetSample(idx, &m_trainSet[pos * dims]);
				m_samples.push_back(idx);
			} else {
				char msg[100];
				snprintf(msg, 100, "Unable to find item: %s, %f, %f ", slide, centX, centY);
				gLogger->LogMsg(EvtLogger::Evt_ERROR, msg);
				result = false;
			}

			// Something is wrong, stop processing
			if( !result )
				break;
		}
	}
	
	// Send result back to client
	//
	size_t bytesWritten = ::write(sock, (result) ? passResp : failResp ,
								((result) ? sizeof(passResp) : sizeof(failResp)) - 1);
	if( bytesWritten != (sizeof(failResp) - 1) )
		result = false;

	// If all's well, train the classifier with the updated training set
	//
	if( result ) {

		if( m_iteration == 0 ) {
			// First iteration init the sampler with the objects
			// selected by the user.
			int	*ptr = &m_samples[0];
			result  = m_sampler->Init(m_samples.size(), ptr);
		}

		m_iteration++;
		result = m_classifier->Train(m_trainSet, m_labels, m_samples.size(), m_dataset->GetDims());

		if( result ) {
			m_curAccuracy = CalcAccuracy();
		}
		// Need to select new samples.
		m_curSet.clear();
	}
	return result;
}






bool Learner::FinalizeSession(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*root = json_object(), *sample = NULL, *sampleArray = NULL,
			*jsonObj = NULL;
	const char *uid = NULL;
	int		idx;
	float	score;
	string 	fileName = m_classifierName + ".h5", fqfn;

	// m_UID's length is 1 greater than UID_LENGTH, So we can
	// always write a 0 there to make strlen safe.
	//
	m_UID[UID_LENGTH] = 0;

	if( strlen(m_UID) == 0 ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(finalize) No active session");
		result = false;
	} else {
		jsonObj = json_object_get(obj, "uid");
		const char *uid = json_string_value(jsonObj);
		if( uid == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(finalize) Unable to decode UID");
			result = false;
		} else if( strncmp(uid, m_UID, UID_LENGTH) != 0 ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(finalize) Invalid UID");
			result = false;		
		}
	}


	if( result ) {
		fqfn = m_dataPath + fileName;
		struct stat buffer;
		if( stat(fqfn.c_str(), &buffer) == 0 ) {
			string 	tag = &m_UID[UID_LENGTH - 3];

			fileName = m_classifierName + "_" + tag + ".h5";
		}
	}

	if( result && root == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Error creating JSON array");
		result = false;
	}

	if( result ) {
		sampleArray = json_array();
		if( sampleArray == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create sample JSON Array");
			result = false;
		}
	}

	if( result ) {
		// Iteration count starts form 0.
		json_object_set(root, "iterations", json_integer(m_iteration));
		json_object_set(root, "filename", json_string(fileName.c_str()));

		// We just return an array of the nuclei database id's, label and iteration when added
		//
		for(int i = 0; i < m_samples.size(); i++) {

			sample = json_object();
			if( sample == NULL ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create sample JSON object");
				result = false;
				break;
			}

			int idx = m_samples[i];
			json_object_set(sample, "id", json_integer(m_ids[i]));
			json_object_set(sample, "label", json_integer(m_labels[i]));
			json_object_set(sample, "iteration", json_integer(m_sampleIter[i]));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}
	}

	if( result ) {
		json_object_set(root, "samples", sampleArray);
		json_decref(sampleArray);

		char *jsonStr = json_dumps(root, 0);
		size_t bytesWritten = ::write(sock, jsonStr, strlen(jsonStr));

		if( bytesWritten != strlen(jsonStr) )
			result = false;
		free(jsonStr);
	}
	json_decref(root);

	// Save the training set
	//
	if( result ) {
		result = SaveTrainingSet(fileName);
	}
	
	// This session is done, clear the UID and cleanup associated
	// data
	//
	memset(m_UID, 0, UID_LENGTH + 1);
	Cleanup();

	return result;
}





bool Learner::Visualize(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*root = json_array(), *sample = NULL, *value = NULL;
	int		strata, groups;

	
	// m_UID's length is 1 greater than UID_LENGTH, So we can
	// always write a 0 there to make strlen safe.
	//
	m_UID[UID_LENGTH] = 0;

	if( strlen(m_UID) == 0 ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(visualize) No active session");
		result = false;
	} else {
		value = json_object_get(obj, "uid");
		const char *uid = json_string_value(value);
		if( uid == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(visualize) Unable to decode UID");
			result = false;
		} else if( strncmp(uid, m_UID, UID_LENGTH) != 0 ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(visualize) Invalid UID");
			result = false;		
		}
	}

	if( result ) {
		value = json_object_get(obj, "strata");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(visualize) Invalid strata");
			result = false;
		} else {		
			strata = json_integer_value(value);
		}
	}
	
	if( result ) {
		value = json_object_get(obj, "groups");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(visualize) Invalid groups");
			result = false;	
		} else {
			groups = json_integer_value(value);
		}
	}	
	
	if( root == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to crate JSON array for visualization");
		result = false;
	}

	if( result ) {
		int	*sampleIdx = NULL, totalSamp;
		float *sampleScores = NULL;

		totalSamp = strata * groups * 2;
		result = m_sampler->GetVisSamples(strata, groups, sampleIdx, sampleScores);

		if( result ) {
			for(int i = 0; i < totalSamp; i++) {
				sample = json_object();

				if( sample == NULL ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create sample JSON object");
					result = false;
					break;
				}

				json_object_set(sample, "slide", json_string(m_dataset->GetSlide(sampleIdx[i])));
				json_object_set(sample, "id", json_integer(0));
				json_object_set(sample, "centX", json_real(m_dataset->GetXCentroid(sampleIdx[i])));
				json_object_set(sample, "centY", json_real(m_dataset->GetYCentroid(sampleIdx[i])));
				json_object_set(sample, "maxX", json_integer(0));
				json_object_set(sample, "maxY", json_integer(0));
				json_object_set(sample, "score", json_real(sampleScores[i]));

				json_array_append(root, sample);
				json_decref(sample);
			}

			if( sampleIdx ) {
				free(sampleIdx);
			}
			if( sampleScores ) {
				free(sampleScores);
			}
		}
	}

	if( result ) {
		char *jsonObj = json_dumps(root, 0);
		size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;
		free(jsonObj);
	}
	json_decref(root);

	return result;
}






bool Learner::UpdateBuffers(int updateSize)
{
	bool	result = true;
	int		*newBuff = NULL, newSize = m_samples.size() + updateSize;

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
		newBuff = (int*)realloc(m_sampleIter, newSize * sizeof(int));
		if( newBuff != NULL )
			m_sampleIter = newBuff;
		else
			result = false;
	}

	if( result ) {
		int 	dims = m_dataset->GetDims();
		float *newFeatBuff = (float*)realloc(m_trainSet, newSize * dims * sizeof(float));
		if( newFeatBuff != NULL )
			m_trainSet = newFeatBuff;
		else
			result = false;
	}
	return result;
}





bool Learner::CancelSession(const int sock, json_t *obj)
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






bool Learner::SaveTrainingSet(string fileName)
{
	bool	result = false;;
	MData 	*trainingSet = new MData();

	if( trainingSet != NULL ) {

		result = trainingSet->Create(m_trainSet, m_samples.size(), m_dataset->GetDims(),
							m_labels, m_ids, NULL, NULL, 0);
	}

	if( result ) {
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Saving training set to: " + m_outPath + fileName);
		result = trainingSet->SaveAs(m_outPath + fileName);
	}
	
	if( trainingSet != NULL )
		delete trainingSet;

	
	return result;
}








bool Learner::ApplyClassifier(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*value;
	const char *trainSetFile = NULL, *dataSetFile = NULL;
	MData	*dataSet = NULL, *trainingSet = NULL;
	Classifier *classifier = NULL;
	int		*results = NULL, *labels, dims;
	float	*test, *train;

	// m_UID's length is 1 greater than UID_LENGTH, So we can
	// always write a 0 there to make strlen safe.
	//
	m_UID[UID_LENGTH] = 0;

	double timing = gLogger->WallTime();

	if( strlen(m_UID) == 0 ) {
		// No session active, load specified training set.
		//result = ApplyGeneralClassifier(sock, obj);
	} else {
		// Session in progress, use current training set.
		value = json_object_get(obj, "uid");
		const char *uid = json_string_value(value);
		if( uid == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplyClassifier) Unable to decode UID");
			result = false;
		} else if( strncmp(uid, m_UID, UID_LENGTH) != 0 ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplyClassifier) Invalid UID");
			result = false;		
		}

		if( result ) {
			result = ApplySessionClassifier(sock, obj);
		}
	}

	timing = gLogger->WallTime() - timing;
	char msg[100];

	snprintf(msg, 100, "Classification took: %f", timing);
	gLogger->LogMsg(EvtLogger::Evt_INFO, msg);

	return result;
}









//
//	Applies the specified classifier to the specified dataset. Used when
//	no session is active.
//
bool Learner::ApplyGeneralClassifier(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*value;
	const char *trainSetName = NULL, *dataSetFileName = NULL;
	Classifier *classifier = NULL;
	int		*results = NULL, *labels = NULL;

	value = json_object_get(obj, "dataset");
	dataSetFileName = json_string_value(value);

	value = json_object_get(obj, "trainingset");
	trainSetName = json_string_value(value);

	if( dataSetFileName == NULL || trainSetName == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplyGeneralClassifier) Dataset or Training set not specified");
		result = false;
	}

	if( result ) {
		result = LoadDataset(dataSetFileName);
	}

	if( result ) {
		result = LoadTrainingSet(trainSetName);
	}

	if( result ) {
		results = (int*)malloc(m_dataset->GetNumObjs() * sizeof(int));
		if( results == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplyGeneralClassifier) Unable to allocate results buffer");
			result = false;
		}
	}

	if( result ) {
 		classifier = new OCVBinarySVM();
		if( classifier == NULL ) {
			result = false;
		}
	}

	if( result ) {
		float 	**ptr, *test, *train;
		int 	dims;

		ptr = m_classTrain->GetData();
		train = ptr[0];
		labels = m_classTrain->GetLabels();

		ptr = m_dataset->GetData();
		test = ptr[0];
		dims = m_dataset->GetDims();

		result = classifier->Train(train, labels, m_classTrain->GetNumObjs(), dims);
		if( result ) {
			result = classifier->ClassifyBatch(test, m_dataset->GetNumObjs(), dims, results);
		}
	}

	if( result ) {
		result = SendClassifyResult(0, 0, 0, 0, "", results, sock);
	}

	if( results )
		free(results);
	if( classifier )
		delete classifier;

	return result;
}







bool Learner::ApplySessionClassifier(const int sock, json_t *obj)
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
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplySessionClassifier) Unable to decode slide name");
		result = false;
	}

	if( result ) {
		float 	**ptr, *test;
		int 	*labels = (int*)malloc(m_dataset->GetNumObjs() * sizeof(int)), dims;

		if( labels == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplySessionClassifier) Unable to allocate labels buffer");
			result = false;
		}

		if( result ) {
			ptr = m_dataset->GetData();
			test = ptr[0];
			dims = m_dataset->GetDims();

			result = m_classifier->ClassifyBatch(test, m_dataset->GetNumObjs(), dims, labels);
		}

		if( result ) {
			result = SendClassifyResult(xMin, xMax, yMin, yMax, slideName,
										labels, sock);
		}

		if( labels )
			free(labels);
	}
	return result;
}






bool Learner::SendClassifyResult(int xMin, int xMax, int yMin, int yMax,
								 string slide, int *results, const int sock)
{
	bool	result = true;
	json_t	*root = json_object();

	if( root == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(SendClassifyResult) Unable to create JSON object");
		result = false;
	}

	if( result ) {
		char tag[25];

		for(int i = 0; i < m_dataset->GetNumObjs(); i++) {

			if( slide.compare(m_dataset->GetSlide(i)) == 0 &&
				m_dataset->GetXCentroid(i) >= xMin && m_dataset->GetXCentroid(i) <= xMax &&
				m_dataset->GetYCentroid(i) >= yMin && m_dataset->GetYCentroid(i) <= yMax ) {

				snprintf(tag, 24, "%.1f_%.1f",
						m_dataset->GetXCentroid(i), m_dataset->GetYCentroid(i));

				json_object_set(root, tag, json_integer((results[i] == -1) ? 0 : 1 ));
			}
		}

		if( result ) {
			gLogger->LogMsg(EvtLogger::Evt_INFO, "(SendClassifyResult) Dumping JSON object");
			char *jsonObj = json_dumps(root, 0);

			if( jsonObj == NULL ) {
				result = false;
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(SendClassifyResult) Unable to encode JSON");
			}

			if( result ) {
				gLogger->LogMsg(EvtLogger::Evt_INFO, "(SendClassifyResult) Sending JSON object");
				size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

				if( bytesWritten != strlen(jsonObj) ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "(SendClassifyResult) Unable to send entire JSON object");
					result = false;
				}
			}
			json_decref(root);
			free(jsonObj);
		}
	}
	return result;
}







bool Learner::LoadDataset(string dataSetFileName)
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
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Loading: " + fqn);
			result = m_dataset->Load(fqn);
		}
	}
	return result;
}






bool Learner::LoadTrainingSet(string trainingSetName)
{
	bool	result = true;

	// Only load if not already loaded
	//
	if( m_classifierName.compare(trainingSetName) != 0 ) {
		m_classifierName = trainingSetName;

		if( m_classTrain )
			delete m_classTrain;

		m_classTrain = new MData();
		if( m_classTrain == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(LoadTrainingSet) Unable to create trainset object");
			result = false;
		}

		if( result ) {
			string fqn = m_outPath + trainingSetName + ".h5";
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Loading: " + fqn);
			result = m_classTrain->Load(fqn);
		}
	}
	return result;
}





#define FOLDS	5

//
//	Calculate the in-sample accuracy using the current
//	training set.
//
float Learner::CalcAccuracy(void)
{
	float		result = -1.0f;
	Classifier	*classifier = new OCVBinarySVM();

	if( classifier ) {
		vector<int>::iterator  it;

		classifier->Train(m_trainSet, m_labels, m_samples.size(), m_dataset->GetDims());

		int  *results = (int*)malloc(m_samples.size() * sizeof(int));
		if( results ) {
			classifier->ClassifyBatch(m_trainSet, m_samples.size(), m_dataset->GetDims(), results);
			int  score = 0;
			for(int i = 0; i < m_samples.size(); i++) {
				if( results[i] == m_labels[i] )
					score++;
			}
			result = (float)score / (float)m_samples.size();
			free(results);
		}
		delete classifier;
	}

	// Randomize the data the split into 5 folds
	// foldList contains the fold the corresponding data object belongs to
	vector<int> foldList;

	for(int i = 0; i < m_samples.size(); i++) {
			foldList.push_back(i % FOLDS);
	}
	random_shuffle(foldList.begin(), foldList.end());

	float *trainX = NULL, *testX = NULL;
	int	*trainY = NULL, *testY = NULL;


	for(int fold = 0; fold < FOLDS; fold++) {
		if( CreateSet(foldList, fold, trainX, trainY, testX, testY) ) {

		} else {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create training / test set for validation");
			result = false;
			break;
		}
	}

	return result;
}







bool Learner::CreateSet(vector<int> folds, int fold, float *&trainX, int *&trainY, float *&testX, int *&testY)
{
	bool	result = true;


	return result;
}
