#include <cstdlib>
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
m_dataPath(dataPath)
{
	memset(m_UID, 0, UID_LENGTH + 1);
	m_samples.clear();
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

			if( strncmp(command, "init", 4) == 0 ) {
				result = StartSession(sock, root);
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
		result = false;
	}

	if( result ) {
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
		string fqFileName = m_dataPath + string(fileName);
		result = m_dataset->Load(fqFileName);
	}

	// Create sampling objects
	if( result ) {
		m_classifier = new OCVBinarySVM();
		if( m_classifier == NULL )
			result = false;
	}

	if( result ) {
		m_sampler = new UncertainSample(m_classifier, m_dataset);
		if( m_sampler == NULL )
			result = false;
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

	if( m_samples.empty() ) {
		// TEMPORARY!!!!
		// For now prime is hard coded to put predetermined
		// objects int the set. When the "prime" functionality is
		// implemented, prime will be called with the selected objects.
		result = Prime(sock, obj);
	} else {
		json_t	*root = json_object(), *sample = NULL, *sampleArray = NULL;
		int		idx;
		float	score;


		cout << "Iteration " << m_iteration << endl;
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

			for(int i = 0; i < 8; i++) {

				sample = json_object();
				if( sample == NULL ) {
					cerr << "Unable to create sample JSON object" << endl;
					result = false;
					break;
				}

				// Get new sample
				idx = m_sampler->Select(&score);
				cout << "Selected " << idx << endl;
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
	}
	return result;
}





// ********************* TO BE CHANGED **************************************
//	Note - Until the prime screens are done in the webapp, This function
//  	will be used to supply the inital set of samples. The initial set is
//		hardcoded here.
//
//	This method will change to just receive the samples that were selected
//	by th user.
// ********************* TO BE CHANGED **************************************
bool Learner::Prime(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*root = json_object(), *sampleArray = NULL, *sample = NULL;

	// This is the first time through
	m_iteration = 1;

	cout << "!!!!!! Sending initial set !!!!!" << endl;


	// Root array contains the iteration and the sample array
	if( root == NULL ) {
		cerr << "Error creating root JSON object" << endl;
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
	}

	// First sample object
	//
	if( result ) {
		sample = json_object();
		if( sample == NULL ) {
			cerr << "Unable to create sample JSON object" << endl;
			result = false;
		}
	}

	if( result ) {
		json_object_set(sample, "slide", json_string("TCGA-08-0520-01Z-00-DX1"));
		json_object_set(sample, "id", json_integer(280003));
		json_object_set(sample, "centX", json_real(28676.3));
		json_object_set(sample, "centY", json_real(9239.2));
		json_object_set(sample, "label", json_integer(1));
		json_object_set(sample, "maxX", json_integer(0));
		json_object_set(sample, "maxY", json_integer(0));
		json_object_set(sample, "boundary", json_string(""));

		json_array_append(sampleArray, sample);
		json_decref(sample);

		sample = json_object();
		if( sample != NULL ) {
			json_object_set(sample, "slide", json_string("TCGA-08-0520-01Z-00-DX1"));
			json_object_set(sample, "id", json_integer(280079));
			json_object_set(sample, "centX", json_real(28680.9));
			json_object_set(sample, "centY", json_real(9193.0));
			json_object_set(sample, "label", json_integer(1));
			json_object_set(sample, "maxX", json_integer(0));
			json_object_set(sample, "maxY", json_integer(0));
			json_object_set(sample, "boundary", json_string(""));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}

		sample = json_object();
		if( sample != NULL ) {
			json_object_set(sample, "slide", json_string("TCGA-08-0520-01Z-00-DX1"));
			json_object_set(sample, "id", json_integer(280188));
			json_object_set(sample, "centX", json_real(28695.2));
			json_object_set(sample, "centY", json_real(9431.4));
			json_object_set(sample, "label", json_integer(1));
			json_object_set(sample, "maxX", json_integer(0));
			json_object_set(sample, "maxY", json_integer(0));
			json_object_set(sample, "boundary", json_string(""));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}

		sample = json_object();
		if( sample != NULL ) {
			json_object_set(sample, "slide", json_string("TCGA-08-0520-01Z-00-DX1"));
			json_object_set(sample, "id", json_integer(223709));
			json_object_set(sample, "centX", json_real(28658.7));
			json_object_set(sample, "centY", json_real(9340.5));
			json_object_set(sample, "label", json_integer(1));
			json_object_set(sample, "maxX", json_integer(0));
			json_object_set(sample, "maxY", json_integer(0));
			json_object_set(sample, "boundary", json_string(""));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}

		sample = json_object();
		if( sample != NULL ) {
			json_object_set(sample, "slide", json_string("TCGA-08-0520-01Z-00-DX1"));
			json_object_set(sample, "id", json_integer(376177));
			json_object_set(sample, "centX", json_real(33315.6));
			json_object_set(sample, "centY", json_real(21492.3));
			json_object_set(sample, "label", json_integer(-1));
			json_object_set(sample, "maxX", json_integer(0));
			json_object_set(sample, "maxY", json_integer(0));
			json_object_set(sample, "boundary", json_string(""));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}

		sample = json_object();
		if( sample != NULL ) {
			json_object_set(sample, "slide", json_string("TCGA-08-0520-01Z-00-DX1"));
			json_object_set(sample, "id", json_integer(376216));
			json_object_set(sample, "centX", json_real(33325.4));
			json_object_set(sample, "centY", json_real(21677.4));
			json_object_set(sample, "label", json_integer(-1));
			json_object_set(sample, "maxX", json_integer(0));
			json_object_set(sample, "maxY", json_integer(0));
			json_object_set(sample, "boundary", json_string(""));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}

		sample = json_object();
		if( sample != NULL ) {
			json_object_set(sample, "slide", json_string("TCGA-08-0520-01Z-00-DX1"));
			json_object_set(sample, "id", json_integer(150545));
			json_object_set(sample, "centX", json_real(17490.2));
			json_object_set(sample, "centY", json_real(21124.8));
			json_object_set(sample, "label", json_integer(-1));
			json_object_set(sample, "maxX", json_integer(0));
			json_object_set(sample, "maxY", json_integer(0));
			json_object_set(sample, "boundary", json_string(""));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}

		sample = json_object();
		if( sample != NULL ) {
			json_object_set(sample, "slide", json_string("TCGA-08-0520-01Z-00-DX1"));
			json_object_set(sample, "id", json_integer(190362));
			json_object_set(sample, "centX", json_real(27021.3));
			json_object_set(sample, "centY", json_real(5192.4));
			json_object_set(sample, "label", json_integer(-1));
			json_object_set(sample, "maxX", json_integer(0));
			json_object_set(sample, "maxY", json_integer(0));
			json_object_set(sample, "boundary", json_string(""));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}

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


	value = json_object_get(obj, "iteration");
	iter = json_integer_value(value);
	cout << "Iteration: " << iter << ", m_iteration: " << m_iteration << endl;

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

			// The dynamic typing in PHP & javascript can make a float
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
	}
	return result;
}






bool Learner::FinalizeSession(const int sock, json_t *obj)
{
	bool	result = true;


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

	int 	dims = m_dataset->GetDims();
	float *newFeatBuff = (float*)realloc(m_trainSet, newSize * dims * sizeof(float));
	if( newFeatBuff != NULL )
		m_trainSet = newFeatBuff;
	else
		result = false;

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

