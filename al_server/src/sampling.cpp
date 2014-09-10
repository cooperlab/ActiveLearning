#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>


#include "sampler.h"
#include "infodensity.h"
#include "hierarchsample.h"
#include "clustersample.h"
#include "data.h"
#include "ocvsvm.h"

#include "cmdline.h"
#include "base_config.h"


using namespace std;

// Set to 1 to use support set version of cluster sampling
#define SUPPORT_SET 0


#define INITIAL_SET  5



int	gSeed = -1;
fstream		*gInitial = NULL;
bool		gSaveInit = false;
bool		gReadInit = false;



bool InitTraining(MData *dataset, Sampler *sampler,
				  vector<int>& samples, float *&trainSet,
				  int *&labels)
{
	bool	result = false;
	int		firstSamp[INITIAL_SET], dims, *dataLabels;
	float	**data;

	// Empty the list of chosen samples
	samples.clear();

	dims = dataset->GetDims();
	trainSet = (float*)malloc(INITIAL_SET * dims * sizeof(float));
	labels = (int*)malloc(INITIAL_SET * sizeof(int));

	dataLabels = dataset->GetLabels();
	data = dataset->GetData();

	if( trainSet && labels ) {
		if( gReadInit )
			result = true;
		else
			result = sampler->Init(INITIAL_SET, firstSamp);

		if( result ) {
			for(int i = 0; i < INITIAL_SET; i++) {
				if( gReadInit )
					*gInitial >> firstSamp[i];

				samples.push_back(firstSamp[i]);
				memcpy(&trainSet[i * dims], data[firstSamp[i]], dims * sizeof(float));
				labels[i] = dataLabels[firstSamp[i]];

				if( gSaveInit && gInitial->is_open() )
					*gInitial << firstSamp[i] << " ";
			}
			if( gSaveInit && gInitial->is_open() )
				*gInitial << endl;

#if SUPPORT_SET
			// Cluster sampling keeps a set of sampled clusters, need to initialize
			// when using a predetermined initial set
			//
			if( gReadInit && sampler->GetSamplerType() == Sampler::SAMP_CLUSTER )
				result = ((ClusterSample*)sampler)->SetSupportSet(firstSamp, INITIAL_SET);
#endif
		}
	}
	return result;
}







// Add a new sample to the training set.
//
bool UpdateTraining(MData *dataset, Sampler *sampler, vector<int>& samples,
					float *&trainSet, int *&labels, bool dual)
{
	bool	result = true;
	int		oldSize = samples.size(), *newLabels = NULL,
			dims = dataset->GetDims(), newSample , *dataLabels,
			otherSample = -1;
	float	*newSet = NULL, **data = dataset->GetData();

	dataLabels = dataset->GetLabels();

	if( dual )
		newSample = sampler->Select(otherSample);
	else
		newSample = sampler->Select();
#if SUPPORT_SET
	set<int>  	supportSet;
	int			*supportLabels = NULL;
	if( sampler->GetSamplerType() == Sampler::SAMP_CLUSTER )
		((ClusterSample*)sampler)->GetSupportSet(supportSet, supportLabels);
#endif

	if( newSample == -1 ) {
		cerr << "sampler->Select failed" << endl;
		result = false;
	}

	if( result ) {
		samples.push_back(newSample);
		if( otherSample != -1 )
			samples.push_back(otherSample);

		// Adjust buffers
#if SUPPORT_SET
		int	newSize = samples.size() + supportSet.size();
#else
		int	newSize = samples.size();
#endif

		newSet = (float*)realloc(trainSet, newSize * dims * sizeof(float));
		if( newSet )
			trainSet = newSet;
		else
			result = false;

		if( result ) {
			newLabels = (int*)realloc(labels, newSize * sizeof(int));
			if( newSet )
				labels = newLabels;
			else
				result = false;
		}
	}

	if( result ) {
		// Add new sample to the training set
		memcpy(&trainSet[oldSize * dims], data[newSample], dims * sizeof(float));
		labels[oldSize] = dataLabels[newSample];

		if( otherSample != -1 ) {
			oldSize++;
			// Add second sample to the training set
			memcpy(&trainSet[oldSize * dims], data[otherSample], dims * sizeof(float));
			labels[oldSize] = dataLabels[otherSample];
		}
#if SUPPORT_SET
		if( supportSet.size() > 0 ) {
			set<int>::iterator it;
			int		idx = 0;

			for(it = supportSet.begin(); it != supportSet.end(); it++) {
				oldSize++;
				memcpy(&trainSet[oldSize * dims], data[*it], dims * sizeof(float));
				labels[oldSize] = supportLabels[idx++];
			}
		}
#endif
	}
	return result;
}





//
//
//
bool InitHierarchical(MData *dataset, Sampler *sampler,
				  vector<int>& samples, float *&trainSet,
				  int *&labels)
{
	bool	result = false;
	int		numObjs = dataset->GetNumObjs(),
			dims = dataset->GetDims(),
			firstSamp[INITIAL_SET];

	labels = (int*)malloc(numObjs * sizeof(int));

	if( labels ) {

//		sampler->Init(INITIAL_SET, firstSamp);

		for(int i = 0; i < INITIAL_SET; i++) {
			samples.push_back(i);
		}

		trainSet = dataset->GetData()[0];
		string fileName = "out.";
		char numBuffer[10];
		snprintf(numBuffer, 9, "%d", INITIAL_SET);
		fileName += numBuffer;

		ifstream	inFile(fileName, ios::in);

		int node, label;
		for(int i = 0; i < dataset->GetNumObjs(); i++) {
			inFile >> node;
			inFile >> label;
			labels[node] = (label == 1) ? 1 : -1;
		}
		inFile.close();
		result = true;
	}
	return result;
}




//
// Hierarchical sampling always uses the entire training set. Lables are
// determined using the queried labels. The samples vector is used to keep
// track of the selected objects for information purposes. There is no other
// use for them as with the other sampling methods.
//
bool UpdateHierarchical(MData *dataset, Sampler *sampler, vector<int>& samples,
						float *&trainSet, int *&labels)
{
	bool	result = true;

	samples.push_back(0);	// Just add a sample

	string fileName = "out.";
	char numBuffer[10];
	snprintf(numBuffer, 9, "%zu", samples.size());
	fileName += numBuffer;

	cout << fileName << ": ";
	ifstream	inFile(fileName, ios::in);

	int node, label;
	for(int i = 0; i < dataset->GetNumObjs(); i++) {
		inFile >> node;
		inFile >> label;
		labels[node] = (label == 1) ? 1 : -1;
	}
	inFile.close();

	return result;
}





float CalcAccuracy(int *labels, int *results, int numObjs)
{
	int	count = 0;

	for(int i = 0; i < numObjs; i++) {
		if( labels[i] == results[i] )
			count++;
	}
	return (float)count / (float)numObjs;
}







// Change to 1 to log sample number, entropy and accuracy
#define LOG_SAMPLES 0

// Change to 1 to log the entropy of the dataset
#define LOG_ENTROPY 0



void RunExp(MData *dataset, MData *testData, Sampler *sampler, bool dual,
			Classifier *classify, string outFile, int runs, int maxSamples)
{
	int		*results = NULL, *labels = NULL;
	float 	*trainSet = NULL, *acc = NULL;
	bool	prog = true, logging = !outFile.empty();

	// Buffer for test results
	//
	results = (int*)malloc(testData->GetNumObjs() * sizeof(int));
	if( results == NULL ) {
		cerr << "Unable to allocate results buffer" << endl;
		prog = false;
	}

	// Buffer to store accuracy for each iteration.
	if( prog ) {
		 acc = (float*)malloc((maxSamples - INITIAL_SET) * sizeof(float));
		if( acc == NULL ) {
			cerr << "Unable to allocate accuracy buffer" << endl;
			prog = false;
		}
	}

	int accIdx;
	vector<int> samples;
	ofstream 	out;
#if LOG_SAMPLES
	ofstream log("sample_log.txt", ofstream::out);
#endif
#if LOG_ENTROPY
	ofstream entLog("entropy_log.txt", ofstream::out);
	float	 *entScores = (float*)malloc(dataset->GetNumObjs() * sizeof(float));
#endif

	if( logging ) {
		out.open(outFile.c_str(), ofstream::out);

		// Write header
		out << "run, ";
		for(int i = INITIAL_SET; i < maxSamples - 1; i++)
			out << "s=" << i << ", ";
		out << "s=" << maxSamples - 1 << endl;

		cout << "Run: " << flush;
	}
#if LOG_SAMPLES
	log << "Samples, obj, entropy, accuracy" << endl;
#endif
	for(int run = 0; run < runs; run++) {

		if( logging == false ) {
			cout << "=================== Run: " << run << " =========================" << endl;
		} else {
			cout << run + 1 << ", " << flush;
		}

		// Init training set with a few objects
		if( prog ) {
			if( sampler->GetSamplerType() == Sampler::SAMP_HIERARCHICAL ) {
				prog = InitHierarchical(dataset, sampler, samples, trainSet, labels);
			} else {
				prog = InitTraining(dataset, sampler, samples, trainSet, labels);
			}
		}

		vector<int>::iterator it;
		if( prog ) {
			cout << "Initial objects: " << endl;
			for(it = samples.begin(); it != samples.end(); it++)
				printf("%3d ", *it);
			cout << endl;
		}



#if LOG_SAMPLES
		log << "Initial objects: " << endl;
		for(int i = 0; i < samples.size() - 1; i++)
			log << samples[i] << ", ";
		log << samples[samples.size() - 1] << ", accuracy: ";
#endif

		if( prog ) {
			int	lastSampleCount = samples.size();
			accIdx = 0;
			// Loop until specified number of samples are used to train
			while( samples.size() < maxSamples ) {

#if 0
				vector<int>::iterator sampIt;
				cout << "Samples: ";
				for(sampIt = samples.begin(); sampIt != samples.end(); sampIt++)
					cout << *sampIt << " ";
				cout << endl;
#endif


				if( logging == false ) {
					cout << "++++++++++++ Samples: " << samples.size() << " +++++++++++++++++++" << endl;
					cout << "Training..." << endl;
				}

				if( sampler->GetSamplerType() == Sampler::SAMP_HIERARCHICAL ) {
					// Use complete trainin set always. samples.size() will indicate
					// the number of samples used to determine the labels
					prog = classify->Train(trainSet, labels, dataset->GetNumObjs(), dataset->GetDims());
				} else {
					prog = classify->Train(trainSet, labels, samples.size(), dataset->GetDims());
				}

				if( logging == false )
					cout << "Testing..." << endl;

				if( prog ) {
					float	**data = testData->GetData();
					prog = classify->ClassifyBatch(data[0], testData->GetNumObjs(), testData->GetDims(), results);
				}

				if( prog ) {
					acc[accIdx] = CalcAccuracy(testData->GetLabels(), results, testData->GetNumObjs());
					int diff = samples.size() - lastSampleCount;

					while( diff > 1 ) {
						accIdx++;
						acc[accIdx] = acc[accIdx - 1];
						diff--;
					}
//					if( dual && accIdx > 0 ) {
//						accIdx++;
//						acc[accIdx] = acc[accIdx - 1];
//					}

					if( logging == false )
						cout << "Sample count: " << samples.size() << " accuracy: " << "\033[1;35m" << acc[accIdx] * 100.0f << "\033[0m" << endl;

#if LOG_SAMPLES
					log << acc[accIdx] * 100.0f << endl;
#endif
#if LOG_ENTROPY
					float **data = dataset->GetData();
					int		entObjs = dataset->GetNumObjs();
					if( classify->ScoreBatch(data[0], entObjs,
										dataset->GetDims(), entScores) )
					{
						for(int i = 0; i < entObjs - 1; i++)
							entLog << entScores[i] << ", ";
						entLog << entScores[entObjs - 1] << endl;
					}
#endif
				}

				if( prog && samples.size() < maxSamples ) {
					lastSampleCount = samples.size();

					if( sampler->GetSamplerType() == Sampler::SAMP_HIERARCHICAL ) {
						prog = UpdateHierarchical(dataset, sampler, samples, trainSet, labels);
					} else {
						prog = UpdateTraining(dataset, sampler, samples, trainSet, labels, dual);
					}
				}
#if LOG_SAMPLES
				int sel = samples.back(), tIdx = samples.size() - 1;
				float score = classify->Score(&trainSet[tIdx * dataset->GetDims()], dataset->GetDims());
				log << iter << ":, " << sel << ", " << score << ", ";
#endif
				if( prog == false )
					break;
				accIdx++;
			}
		}

		if( logging ) {
			// Write accuracy for the run
			out << run << ":, ";
			for(int i = 0; i < maxSamples - INITIAL_SET - 1; i++)
				out << acc[i] << ", ";
			out << acc[maxSamples - INITIAL_SET - 1] << endl;
		}


		if( trainSet && sampler->GetSamplerType() != Sampler::SAMP_HIERARCHICAL )
			free(trainSet);
		if( labels )
			free(labels);

		// Reset the sampler so the whole dataset is available
		sampler->Reset();

	}
#if LOG_SAMPLES
	log.close();
#endif

#if LOG_ENTROPY
	entLog.close();
	if( entScores )
		free(entScores);
#endif
	if( logging ) {
		out.close();
		cout << endl << flush;
	}
	if( results )
		free(results);
	if( acc )
		free(acc);
}





struct obj {
	int		index;
	float 	score;
};

struct compClass {
	bool operator() (obj i, obj j) { return (abs(i.score) < abs(j.score)); }
}compObj;




int main(int argc, char *argv[])
{
	int 			result = 0;
	gengetopt_args_info	args;

	if( cmdline_parser(argc, argv, &args) != 0 ) {
		exit(-1);
	}

	string sampleMethod = args.method_arg,
		   dataFile = args.data_file_arg,
		   testFile = args.test_file_arg,
		   outFile = "";
	int	  runs = args.runs_arg, k = -1;
	bool  dual = false;

	if( args.out_file_given )
		outFile = args.out_file_arg;

	if( args.seed_given )
		gSeed = args.seed_arg;

	if( args.dual_given && args.dual_flag == 1 )
		dual = true;

	if( args.save_initial_given == 1 ) {
		gSaveInit = true;
		gInitial = new fstream(args.save_initial_arg, ios::out);
	} else if( args.use_initial_given == 1 ) {
		gReadInit = true;
		gInitial = new fstream(args.use_initial_arg, ios::in);
	}

	if( args.clusters_given == 1 )
		k = args.clusters_arg;

	cout << "Sample using: " << sampleMethod << endl;

	Classifier	*curClassify = NULL;
	MData		*dataset = new MData(), *testData = new MData();

	if( dataset && testData ) {
		//BinarySVM	*svm = new BinarySVM();		// Only using SVM for now
		OCVBinarySVM *svm = new OCVBinarySVM();
		//MLibSVM 	*svm = new MLibSVM();
		Sampler		*sampler = NULL;
		int			iterations;

		// TODO - Generalize classifier, don't restrict to SVM, add option

		if( dataset->Load(dataFile) == false ) {
			result = -1;
		} else {
			cout << "Dataset: Loaded " << dataFile << ", " << dataset->GetNumObjs()
				<< " objects, " << dataset->GetDims() << " features" << endl;
		}

		if( result == 0 ) {
			if( testData->Load(testFile) == false ) {
				result = -2;
			} else {
				cout << "Test set: Loaded " << testFile << ", " << testData->GetNumObjs()
					 << " objects, " << testData->GetDims() << " features" << endl;
			}
		}

		if( result == 0 ) {
			if( args.limit_given )
				iterations = args.limit_arg;
			else
				iterations = dataset->GetNumObjs();
		}

		// Create sampling object
		if( result == 0 ) {
			if( sampleMethod.compare("random") == 0 ) {
				RandomSample  *randSmp = new RandomSample(dataset);
				sampler = randSmp;

			} else if( sampleMethod.compare("uncertain") == 0 ) {
				UncertainSample *uncertSamp = new UncertainSample(svm, dataset);
				sampler = uncertSamp;

			} else if( sampleMethod.compare("info_density") == 0 ) {
				InfoDensitySample *infoDen = new InfoDensitySample(svm, dataset, args.beta_arg);
				sampler = infoDen;

			} else if( sampleMethod.compare("hierarchical") == 0 ) {
				HierarchicalSample	*hierSamp = new HierarchicalSample(dataset);
				sampler = hierSamp;

			} else if( sampleMethod.compare("cluster") == 0 ) {

				if( k > 0 ) {
					ClusterSample	*clusterSamp = new ClusterSample(svm, dataset, k);
					sampler = clusterSamp;
				} else {

					cerr << "\033[1;31m" << "Must specify number of clusters ( k > 0 )" << "\033[0m" << endl;
					result = -1;
				}
			}
		}

		// Run experiments
		if( sampler )
			RunExp(dataset, testData, sampler, dual, svm, outFile, runs, iterations);

		if( svm )
			delete svm;
		if( sampler )
			delete sampler;
	}

	if( dataset )
		delete dataset;
	if( testData )
		delete testData;
	if( gInitial )
		delete gInitial;

	return result;
}
