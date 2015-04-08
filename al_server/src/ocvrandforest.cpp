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
#include <thread>
#include <algorithm>
#include <ctime>

#include "ocvrandforest.h"


using namespace std;



OCVBinaryRF::OCVBinaryRF(void)
{
	m_priors[0] = 1.0f;		// Each class is of equal importance
	m_priors[1] = 1.0f;

	// Seed random number generator
	CvRNG* rng = m_RF.get_rng();
	*rng = time(NULL);

	m_params.max_depth = 10;
	m_params.min_sample_count = 5;
	m_params.regression_accuracy = 0;
	m_params.use_surrogates = false;
	m_params.max_categories = 2;
	m_params.priors = m_priors;
	m_params.calc_var_importance = false;
	m_params.nactive_vars = 5;

	float 	forestAccuracy = 0.01f;
	int		maxTrees = 100;

	m_params.term_crit = cvTermCriteria(CV_TERMCRIT_ITER, maxTrees, forestAccuracy);
}








OCVBinaryRF::~OCVBinaryRF(void)
{

}






bool OCVBinaryRF::Train(float *trainSet, int *labelVec,
					  int numObjs, int numDims)
{
	Mat		features(numObjs, numDims, CV_32F, trainSet),
			labels(numObjs, 1, CV_32S, labelVec);

	m_params.nactive_vars = floor(sqrtf(numDims));
	m_params.min_sample_count = (int)max(1.0f, ceil((float)numObjs * 0.01f));
	m_trained = m_RF.train(features, CV_ROW_SAMPLE, labels, Mat(), Mat(), Mat(), Mat(), m_params);

	return m_trained;
}







int OCVBinaryRF::Classify(float *obj, int numDims)
{
	int	result = 0;

	if( m_trained ) {
		Mat 	sample(1, numDims, CV_32F, obj);

		result = (int)m_RF.predict(sample);
	}
	return result;
}






bool OCVBinaryRF::ClassifyBatch(float **dataset, int numObjs,
								  int numDims, int *results)
{
	bool	result = false;

	if( m_trained && results != NULL ) {
		for(int i = 0; i < numObjs; i++) {
			Mat	data(1, numDims, CV_32F, dataset[i]);
			results[i] = (int)m_RF.predict(data);
		}
		result = true;
	}
	return result;
}






float OCVBinaryRF::Score(float *obj, int numDims)
{
	float	score = 0.0;

	if( m_trained ) {
		Mat 	sample(1, numDims, CV_32F, obj);

		score = m_RF.predict_prob(sample);
		// Returned a probability, center 50% at 0 and set range to -1 to 1
		score = (score * 2.0f) - 1.0f;
	}
	return score;
}





bool OCVBinaryRF::ScoreBatch(float **dataset, int numObjs,
							int numDims, float *scores)
{
	bool	result = false;

	if( m_trained && scores != NULL ) {
		vector<std::thread> workers;
		unsigned	numThreads = thread::hardware_concurrency();
		int			offset, objCount, remain, objsPer;

		remain = numObjs % numThreads;
		objsPer = numObjs / numThreads;

		// numThreads - 1 because main thread (current) will process also
		//
		for(int i = 0; i < numThreads - 1; i++) {

			objCount = objsPer + ((i <  remain) ? 1 : 0);
			offset = (i < remain) ? i * (objsPer + 1) :
						(remain * (objsPer + 1)) + ((i - remain) * objsPer);

			workers.push_back(std::thread(&OCVBinaryRF::ScoreWorker, this,
							  std::ref(dataset), offset, objCount, numDims, scores));
		}
		// Main thread's workload
		//
		objCount = objsPer;
		offset = (remain * (objsPer + 1)) + ((numThreads - remain - 1) * objsPer);

		ScoreWorker(dataset, offset, objCount, numDims, scores);

		for( auto &t : workers ) {
			t.join();
		}
		result = true;
	}
	return result;
}




void OCVBinaryRF::ScoreWorker(float **data, int offset, int numObjs, int numDims, float *results)
{

	if( m_trained && results != NULL ) {
		for(int i = offset; i < (offset + numObjs); i++) {
			Mat	object(1, numDims, CV_32F, data[i]);

			results[i] = m_RF.predict_prob(object);

			// Returned a probability, center 50% at 0 and set range to -1 to 1. This way
			// any object with a negative score is in the negative class and a positive
			// score indicates the positive class.
			//
			results[i] = (results[i] * 2.0f) - 1.0f;
		}
	}
}

