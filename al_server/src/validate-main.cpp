/*
 * validate-main.cpp
 *
 *  Created on: Feb 15, 2015
 *      Author: nalisnik
 */
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
	int		*labels = trainSet.GetLabels(), dims = trainSet.GetDims();
	float	**data = trainSet.GetData();

	if( !classifier->Train(data[0], labels, trainSet.GetNumObjs(), dims) ) {
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






int	ClassifySlides(string trainFile, string testFile, Classifier *classifier)
{
	int		result = 0;
	MData	testSet, trainSet;
	int		*posTrainObjs = NULL, *negTrainObjs = NULL,		// Training samples for each slide
			*classes = NULL, *posSlideCnt = NULL, *negSlideCnt = NULL;

	trainSet.Load(trainFile);
	testSet.Load(testFile);

	cout << "Train set: " << trainSet.GetNumObjs() << " nuclei from " << trainSet.GetNumSlides() << " slides" << endl;
	cout << "Test set: " << testSet.GetNumObjs() << " nuclei from " << testSet.GetNumSlides() << " slides" << endl;

	result = CountTrainingObjs(trainSet, testSet, posTrainObjs, negTrainObjs);

	char	**slideNames = trainSet.GetSlideNames();

	for(int i = 0; i < trainSet.GetNumSlides(); i++) {
		cout << slideNames[i] << " -  pos: " << posTrainObjs[i] << ", neg: " << negTrainObjs[i] << endl;
	}

	result = TrainClassifier(classifier, trainSet, 0);
	if( result == 0 ) {
		classes = (int*)malloc(testSet.GetNumObjs() * sizeof(int));
		if( classes == NULL ) {
			cerr << "Unable to allocate results buffer" << endl;
			result = -3;
		}
	}

	if( result == 0 ) {
		float **data = testSet.GetData();
		if( !classifier->ClassifyBatch(data[0], testSet.GetNumObjs(), testSet.GetDims(), classes) ) {
			cerr << "Classification failed" << endl;
			result = -4;
		}
	}

	if( result == 0 ) {
		result = CountResults(testSet, classes, posSlideCnt, negSlideCnt);
	}

	if( result == 0 ) {
		for( int i = 0; i < testSet.GetNumSlides(); i++ ) {
			cout << slideNames[i] << " -> pos: " <<  posSlideCnt[i] << ", neg: " << negSlideCnt[i] << endl;
		}
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


	string 			classType = args.classifier_arg,
					trainFile = args.train_file_arg,
					testFile = args.test_file_arg;
	Classifier		*classifier = NULL;

	if( classType.compare("svm") == 0 ) {
		classifier = new OCVBinarySVM();
	} else if( classType.compare("randForest") == 0 ) {
		classifier = new OCVBinaryRF();
	} else {
		cerr << "Unknown classifier type" << endl;
		result = -2;
	}

	if( result == 0 ) {
		result = ClassifySlides(trainFile, testFile, classifier);
	}

	if( classifier )
		delete classifier;

	return result;
}
