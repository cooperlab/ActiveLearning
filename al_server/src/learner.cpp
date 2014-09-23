#include <cstdlib>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <jansson.h>


#include "learner.h"




using namespace std;

const char passResp[] = "PASS";
const char failResp[] = "FAIL";






Learner::Learner(string dataPath) :
m_dataset(NULL),
m_labels(NULL),
m_ids(NULL),
m_trainSet(NULL),
m_classifier(NULL),
m_sampler(NULL),
m_dataPath(dataPath),
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
	cout << data << endl;

	root = json_loads(data, 0, &error);
	if( !root ) {
		cerr << "Error parsing json" << endl;
		result = false;
	}

	if( result ) {
		json_t	*cmdObj = json_object_get(root, "command");

		if( !json_is_string(cmdObj) ) {
			cerr << "Command is not a string" << endl;
			result = false;
		} else {
			const char	*command = json_string_value(cmdObj);

			// TODO - Get rid of hard-coded commands
			//
			if( strncmp(command, "init", 4) == 0 ) {
				result = StartSession(sock, root);
			} else if( strncmp(command, "prime", 5) == 0 ) {
				result = Submit(sock, root);;
			} else if( strncmp(command, "select", 6) == 0 ) {
				result = this->Select(sock, root);
			} else if( strncmp(command, "end", 3) == 0 ) {
				result = CancelSession(sock, root);
			} else if( strncmp(command, "submit", 6) == 0 ) {
				result = Submit(sock, root);
			} else if( strncmp(command, "finalize", 8) == 0 ) {
				result = FinalizeSession(sock, root);
			}
		}
		json_decref(root);
	}
	return result;
}





bool Learner::StartSession(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*jsonObj;
	const char *fileName, *uid, *classifierName;

	// m_UID's length is 1 greater than UID_LENGTH, So we can
	// always write a 0 there to make strlen safe.
	//
	m_UID[UID_LENGTH] = 0;

	if( strlen(m_UID) > 0 ) {
		cerr << "Session already in progress: " << m_UID << endl;
 		result = false;
	}

	if( result ) {
		m_iteration = 0;
		jsonObj = json_object_get(obj, "features");
		fileName = json_string_value(jsonObj);

		jsonObj = json_object_get(obj, "uid");
		uid = json_string_value(jsonObj);
		if( uid == NULL ) {
			result = false;
		}
	}

	if( result ) {
		strncpy(m_UID, uid, UID_LENGTH);
		m_dataset = new MData();
		if( m_dataset == NULL ) {
			cerr << "Unable to create dataset object" << endl;
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
		cout << "Loading " << fqFileName << endl;

		result = m_dataset->Load(fqFileName);
	}

	// Create sampling objects
	if( result ) {
		m_classifier = new OCVBinarySVM();
		if( m_classifier == NULL ) {
			result = false;
		}
	}

	if( result ) {
		m_sampler = new UncertainSample(m_classifier, m_dataset);
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

	return result;
}






//
// Select new samples for the user to label.
//
//
bool Learner::Select(const int sock, json_t *obj)
{
	bool	result = true;
	json_t 	*value = NULL;
	int		reqIteration;


	value = json_object_get(obj, "iteration");
	reqIteration = json_integer_value(value);

	json_t	*root = json_object(), *sample = NULL, *sampleArray = NULL;
	int		idx;
	float	score;

	if( root == NULL ) {
		cerr << "Error creating JSON array" << endl;
		result = false;
	}

	if( result ) {
		sampleArray = json_array();
		if( sampleArray == NULL ) {
			cerr << "Unable to create sample JSON Array" << endl;
			result = false;
		}
	}

	if( result ) {

		json_object_set(root, "iteration", json_integer(m_iteration));
		json_object_set(root, "accuracy", json_real(m_curAccuracy));

		for(int i = 0; i < 8; i++) {

			sample = json_object();
			if( sample == NULL ) {
				cerr << "Unable to create sample JSON object" << endl;
				result = false;
				break;
			}

			if( m_iteration != reqIteration ) {
				// Get new sample
				idx = m_sampler->Select(&score);
				cout << "Selected " << idx << endl;
				m_curSet.push_back(idx);
				m_curScores.push_back(score);
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
	}

	json_object_set(root, "samples", sampleArray);
	json_decref(sampleArray);

	char *jsonObj = json_dumps(root, 0);
	size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

	if( bytesWritten != strlen(jsonObj) )
		result = false;

	json_decref(root);
	free(jsonObj);

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

	value = json_object_get(obj, "iteration");
	iter = json_integer_value(value);

	// Check iteration to make sure we're in sync
	//
	if( iter != m_iteration ) {
		cerr << "Resubmitting not allowed" << endl;
		result = false;
	}

	if( result ) {
		sampleArray = json_object_get(obj, "samples");
		if( !json_is_array(sampleArray) ) {
			cerr << "Invalid samples array" <<endl;
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

			value = json_object_get(jsonObj, "slide");
			slide = json_string_value(value);

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
				cerr << "Unable to find item" << endl;
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
	json_t	*root = json_object(), *sample = NULL, *sampleArray = NULL;
	int		idx;
	float	score;
	string 	tag = &m_UID[UID_LENGTH - 3];
	string 	fileName = m_classifierName + "_" + tag + ".h5";

	if( root == NULL ) {
		cerr << "Error creating JSON array" << endl;
		result = false;
	}

	if( result ) {
		sampleArray = json_array();
		if( sampleArray == NULL ) {
			cerr << "Unable to create sample JSON Array" << endl;
			result = false;
		}
	}

	if( result ) {
		// The current iteration has not been submitted, hence the -1.
		json_object_set(root, "iterations", json_integer(m_iteration - 1));
		json_object_set(root, "filename", json_string(fileName.c_str()));

		// We just return an array of the nuclei database id's, label and iteration when added
		//
		for(int i = 0; i < m_samples.size(); i++) {

			sample = json_object();
			if( sample == NULL ) {
				cerr << "Unable to create sample JSON object" << endl;
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

	json_object_set(root, "samples", sampleArray);
	json_decref(sampleArray);

	char *jsonObj = json_dumps(root, 0);
	size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

	if( bytesWritten != strlen(jsonObj) )
		result = false;

	json_decref(root);
	free(jsonObj);

	// Save the training set
	//
	result = SaveTrainingSet(fileName);

	// This session is done, clear the UID and cleanup associated
	// data
	//
	memset(m_UID, 0, UID_LENGTH + 1);
	Cleanup();

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

	if( m_samples.size() > 0 ) {
		cout << m_samples.size() << " samples selected" << endl;
		for(int i = 0; i < m_samples.size(); i++) {
			cout << i << ") idx: " << m_samples[i] << ", id: " << m_ids[i] <<
					", label: " << m_labels[i] << endl;

		}
	}

	// Just erase the UID for now
	memset(m_UID, 0, UID_LENGTH + 1);

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
		trainingSet->SaveAs(m_dataPath + fileName);
	}

	if( trainingSet != NULL )
		delete trainingSet;

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
	set<int>	parts[FOLDS];
	int			sampleCnt = m_samples.size(), *items = (int*)malloc(sampleCnt * sizeof(int)),
				foldCnt = sampleCnt / FOLDS, remain = sampleCnt % FOLDS, remainSamples;
	bool		status = true;
	Classifier	*classifier = new OCVBinarySVM();

	// Setup folds by randomly selecting samples for each
	//
	if( items && classifier ) {
		for(int i = 0; i < sampleCnt; i++)
			items[i] = i;

		remainSamples = sampleCnt;

		for(int fold = 0; fold < FOLDS; fold++) {
			int pick, cnt = foldCnt;

			if( fold < remain )
				cnt++;
			for(int i = 0; i < cnt; i++) {
				pick = (int)(rand() % remainSamples);
				parts[fold].insert(items[pick]);

				remainSamples--;
				items[pick] = items[remainSamples];
			}
		}
		status = true;
	}

	if( status ) {
		result = 0.0f;
		set<int>	trainIdx, testIdx;
		float		*train = NULL, *test = NULL;
		int			*trainLabels = NULL, *testLabels = NULL, *results = NULL;

		for(int fold = 0; fold < FOLDS; fold++) {

			trainIdx.clear();
			testIdx.clear();

			for(int i = 0; i < FOLDS; i++) {
				if( i != fold ) {
					trainIdx.insert(parts[i].begin(), parts[i].end());
				} else {
					testIdx.insert(parts[i].begin(), parts[i].end());
				}
			}

			status  = CreateSet(trainIdx, train, trainLabels);
			if( status )
				status = CreateSet(testIdx, test, testLabels);
			if( status ) {
				results = (int*)malloc(testIdx.size() * sizeof(int));
				if( results == NULL ) {
					cerr << "Unable to allocate results buffer" << endl;
					status = false;
				}
			}
			if( status )
				status = classifier->Train(train, trainLabels, trainIdx.size(), m_dataset->GetDims());

			if( status )
				status = classifier->ClassifyBatch(test, testIdx.size(), m_dataset->GetDims(), results);

			if( status ) {
				float	foldAcc = 0.0f;
				for(int i = 0; i < testIdx.size(); i++) {
					if( results[i] == testLabels[i] )
						foldAcc += 1.0f;
				}
				result += (foldAcc / (float)testIdx.size());
			}

			if( train )
				free(train);
			if( test )
				free(test);
			if( trainLabels )
				free(trainLabels);
			if( testLabels )
				free(testLabels);

			if( !status )
				break;
 		}
	}

	if( !status )
		result = -1.0;		// Indicate failure with -1.0
	else
		result /= (float)FOLDS;

	if( items )
		free(items);
	if( classifier )
		delete classifier;

	return result;
}







bool Learner::CreateSet(set<int> dataItems, float *&data, int *&labels)
{
	bool	result = true;
	set<int>::iterator it;
	int		cnt = dataItems.size(), dims = m_dataset->GetDims();

	data = (float*)malloc(cnt * dims * sizeof(float));
	if( data == NULL ) {
		cerr << "[CreateSet] Unable to allocate data buffer" << endl;
		result = false;
	}

	if( result ) {
		labels = (int*)malloc(cnt * sizeof(int));
		if( labels == NULL ) {
			cerr << "[CreateSet] Unable to allocate label buffer" << endl;
			result = false;
		}
	}
	if( result ) {
		int		offset = 0;

		for(it = dataItems.begin(); it != dataItems.end(); it++) {
			memcpy(&data[offset * dims], &m_trainSet[*it * dims], dims * sizeof(float));
			labels[offset] = m_labels[*it];
		}
	}
	return result;
}
