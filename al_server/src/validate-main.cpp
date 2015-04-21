//
//	Copyright (c) 2014-2015, Emory University
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
#include <iostream>
#include <set>

#include "ocvsvm.h"
#include "ocvrandforest.h"
#include "data.h"
#include "logger.h"

#include "val-cmdline.h"
#include "base_config.h"


using namespace std;


EvtLogger	*gLogger = NULL;




int	CalcROC(MData& trainSet, MData& testSet, Classifier *classifier, string testFile)
{
	int		result = 0, *trainLabels = trainSet.GetLabels(),
			*testLabels = testSet.GetLabels(),
			*predClass = (int*)malloc(testSet.GetNumObjs() * sizeof(int));
	float	**train = trainSet.GetData(), **test = testSet.GetData(),
			*scores = (float*)malloc(testSet.GetNumObjs() * sizeof(float));

	if( predClass == NULL || scores == NULL ) {
		result = -10;
		cerr << "Unable to allocate results buffer" << endl;
	}

	if( result == 0 ) {
		classifier->Train(train[0], trainLabels, trainSet.GetNumObjs(), trainSet.GetDims());
		classifier->ScoreBatch(test, testSet.GetNumObjs(), testSet.GetDims(), scores);

		for(int i = 0; i < testSet.GetNumObjs(); i++) {
			if( scores[i] >= 0.0f ) {
				predClass[i] = 1;
			} else {
				predClass[i] = -1;
			}
		}
	}

	int TP = 0, FP = 0, TN = 0, FN = 0, P = 0, N = 0;

	// Calculate confusion matrix
	for(int i = 0; i < testSet.GetNumObjs(); i++) {
		if( testLabels[i] == 1 ) {
			P++;
			if( predClass[i] == 1 ) {
				TP++;
			} else {
				FN++;
			}
		} else {
			N++;
			if( predClass[i] == -1 ) {
				TN++;
			} else {
				FP++;
			}
		}
	}
	printf("\t%4d\t%4d\n", TP, FP);
	printf("\t%4d\t%4d\n\n", FN, TN);

	cout << "FP rate: " << (float)FP / (float)N << endl;
	cout << "TP rate: " << (float)TP / (float)P << endl;
	cout << "Precision: " << (float)TP / (float)(TP + FP) << endl;
	cout << "Accuracy: " << (float)(TP + TN) / (float)(N + P) << endl;

	string fileName = testFile + "-scores.csv";
	unsigned pos = fileName.find_last_of("/");

	fileName = fileName.substr(pos + 1);
	ofstream 	outFile(fileName.c_str());
	if( outFile.is_open() ) {

		cout << "Saving score data to " << fileName << endl;

		outFile << "label,score" << endl;
		for(int idx = 0; idx < testSet.GetNumObjs(); idx++)
			outFile << testLabels[idx] << "," << ((scores[idx] + 1.0f) / 2.0) << endl;

		outFile.close();
	} else {
		cerr << "Unable to create " << fileName << endl;
		result = -11;
	}

	if( predClass )
		free(predClass);
	if( scores )
		free(scores);

	return result;
}




int CountTrainingObjs(MData& trainSet, MData& testSet, int *&posCount, int *&negCount)
{
	int result = 0,
		numTrainSlides = trainSet.GetNumSlides(),
		*slideIdx = trainSet.GetSlideIndices(),
		*labels = trainSet.GetLabels();

	posCount = (int*)calloc(numTrainSlides, sizeof(int));
	negCount = (int*)calloc(numTrainSlides, sizeof(int));

	for(int i = 0; i < trainSet.GetNumObjs(); i++) {
		if( labels[i] == 1 ) {
			posCount[slideIdx[i]]++;
		} else {
			negCount[slideIdx[i]]++;
		}
	}
	return result;
}





int TrainClassifier(Classifier *classifier, MData& trainSet, int iteration)
{
	int		result = 0;
	int		*labels = trainSet.GetLabels(), dims = trainSet.GetDims(), count,
			*iterationList = trainSet.GetIterationList();;
	float	**data = trainSet.GetData();

	count = 0;
	while( iterationList[count] <= iteration && count < trainSet.GetNumObjs() )
		count++;

	cout << "Train set size: " << count  << endl;

	if( !classifier->Train(data[0], labels, count, dims) ) {
		cerr << "Classifier traiing FAILED" << endl;
		result = -10;
	}
	return result;
}





int CountResults(MData& testSet, int *classes, int *&posSlideCnt, int *&negSlideCnt)
{
	int		result = 0;
	int		*slideIdx = testSet.GetSlideIndices();

	posSlideCnt = (int*)calloc(testSet.GetNumSlides(), sizeof(int));
	negSlideCnt = (int*)calloc(testSet.GetNumSlides(), sizeof(int));

	if( negSlideCnt && posSlideCnt ) {
		for(int i = 0; i < testSet.GetNumObjs(); i++) {
			if( classes[i] == 1 ) {
				posSlideCnt[slideIdx[i]]++;
			} else {
				negSlideCnt[slideIdx[i]]++;
			}
		}
	}
	return result;
}






int	ClassifySlides(MData& trainSet, MData& testSet, Classifier *classifier, string testFile)
{
	int		result = 0;
	int		*posTrainObjs = NULL, *negTrainObjs = NULL,		// Training samples for each slide
			*classes = NULL, *posSlideCnt = NULL, *negSlideCnt = NULL;


	result = CountTrainingObjs(trainSet, testSet, posTrainObjs, negTrainObjs);

	char	**slideNames = trainSet.GetSlideNames();

	for(int i = 0; i < trainSet.GetNumSlides(); i++) {
		cout << slideNames[i] << " -  pos: " << posTrainObjs[i] << ", neg: " << negTrainObjs[i] << endl;
	}

	int	lastTrainObjIter = trainSet.GetIteration(trainSet.GetNumObjs() - 1);

	string fileName = testFile + ".csv";
	unsigned pos = fileName.find_last_of("/");

	fileName = fileName.substr(pos + 1);

	ofstream 	outFile(fileName.c_str());

	if( outFile.is_open() ) {
		cout << "Saving to " << fileName << endl;

		outFile << "slides,";
		for(int i = 0; i < trainSet.GetNumSlides() - 1; i++) {
			outFile << slideNames[i] << ",";
		}
		outFile << slideNames[trainSet.GetNumSlides() - 1] << endl;

		if( result == 0 ) {
			classes = (int*)malloc(testSet.GetNumObjs() * sizeof(int));
			if( classes == NULL ) {
				cerr << "Unable to allocate results buffer" << endl;
				result = -3;
			}
		}

		for(int iter = 0; iter <= lastTrainObjIter; iter++) {

			result = TrainClassifier(classifier, trainSet, iter);

			if( result == 0 ) {
				if( !classifier->ClassifyBatch(testSet.GetData(), testSet.GetNumObjs(), testSet.GetDims(), classes) ) {
					cerr << "Classification failed" << endl;
					result = -4;
				}
			}

			if( result == 0 ) {
				result = CountResults(testSet, classes, posSlideCnt, negSlideCnt);
			}

			slideNames = testSet.GetSlideNames();
			if( result == 0 ) {
				outFile << "iter " << iter << "-pos,";
				for( int i = 0; i < testSet.GetNumSlides() - 1; i++ ) {
					 outFile <<  posSlideCnt[i] << ",";
				}
				outFile <<  posSlideCnt[testSet.GetNumSlides() - 1] << endl;

				outFile << "iter " << iter << "-neg,";
				for( int i = 0; i < testSet.GetNumSlides() - 1; i++ ) {
					 outFile <<  negSlideCnt[i] << ",";
				}
				outFile <<  negSlideCnt[testSet.GetNumSlides() - 1] << endl;
			}
		}
	} else {
		cerr << "Unable to create file " << fileName << endl;
	}


	if( posTrainObjs )
		free(posTrainObjs);
	if( negTrainObjs )
		free(negTrainObjs);
	if( posSlideCnt )
		free(posSlideCnt);
	if( negSlideCnt )
		free(negSlideCnt);
	return result;
}







int main(int argc, char *argv[])
{
	int 			result = 0;
	gengetopt_args_info	args;

	if( cmdline_parser(argc, argv, &args) != 0 ) {
		exit(-1);
	}

	gLogger = new EvtLogger();

	MData	testSet, trainSet;
	string 	classType = args.classifier_arg,
			trainFile = args.train_file_arg,
			testFile = args.test_file_arg,
			command = args.command_arg;
	Classifier		*classifier = NULL;


	trainSet.Load(trainFile);
	testSet.Load(testFile);

	cout << "Train set: " << trainSet.GetNumObjs() << " nuclei from " << trainSet.GetNumSlides() << " slides" << endl;
	cout << "Test set: " << testSet.GetNumObjs() << " nuclei from " << testSet.GetNumSlides() << " slides" << endl;


	if( classType.compare("svm") == 0 ) {
		classifier = new OCVBinarySVM();
	} else if( classType.compare("randForest") == 0 ) {
		classifier = new OCVBinaryRF();
	} else {
		cerr << "Unknown classifier type" << endl;
		result = -2;
	}

	if( result == 0 ) {
		if( command.compare("count") == 0 ) {
			result = ClassifySlides(trainSet, testSet, classifier, testFile);
		} else if( command.compare("roc") == 0 ) {
			result = CalcROC(trainSet, testSet, classifier, testFile);
		}
	}

	if( classifier )
		delete classifier;

	return result;
}
