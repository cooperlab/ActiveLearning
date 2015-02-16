/*
 * validate-main.cpp
 *
 *  Created on: Feb 15, 2015
 *      Author: nalisnik
 */
#include <iostream>

#include "ocvsvm.h"
#include "ocvrandforest.h"
#include "data.h"

#include "val-cmdline.h"
#include "base_config.h"


using namespace std;





int CountTraining(MData& trainSet, MData& testSet, int *&posCount, int *&negCount)
{
	int result = 0,
		numTestSlides = testSet.GetNumSlides(),
		numTrainSlides = trainSet.GetNumSlides(),
		*labels = trainSet.GetLabels();

	posCount = (int*)malloc(numTrainSlides * sizeof(int));
	negCount = (int*)malloc(numTrainSlides * sizeof(int));

	for(int i = 0; i < numTrainSlides; i++) {


	}
	return result;
}






int	ClassifySlides(string trainFile, string testFile, Classifier *classifier)
{
	int		result = 0;
	MData	testSet, trainSet;
	int		*posTrain = NULL, *negTrain = NULL;		// Training samples for each slide

	trainSet.Load(trainFile);
	testSet.Load(testFile);

	result = CountTraining(trainSet, testSet, posTrain, negTrain);

	return result;
}







int main(int argc, char *argv[])
{
	int 			result = 0;
	gengetopt_args_info	args;

	if( cmdline_parser(argc, argv, &args) != 0 ) {
		exit(-1);
	}

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
