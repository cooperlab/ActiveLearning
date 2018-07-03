//
//	Copyright (c) 2014-2018, Emory University
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

#include "ocvnn.h"
#include "logger.h"


using namespace std;

extern EvtLogger	*gLogger;

OCVBinaryNN::OCVBinaryNN(void)
{
	m_priors[0] = 0.5f;		// Priors will be updated during training
	m_priors[1] = 0.5f;

	float 	eps = 0.00001;
	int		maxIteration = 1000;

	m_params.term_crit = cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, maxIteration, eps);
	m_params.train_method = CvANN_MLP_TrainParams::BACKPROP;
	m_params.bp_dw_scale = 0.1;
	m_params.bp_moment_scale = 0.1;

  int attributes_per_sample = 64;
	int hidden_samples = 32;
  int number_of_classes = 2;
	Mat layers = Mat(3, 1, CV_32SC1);
	layers.row(0) = Scalar(attributes_per_sample);
	layers.row(1) = Scalar(hidden_samples);
	layers.row(2) = Scalar(number_of_classes);
	m_NN.create(layers, CvANN_MLP::SIGMOID_SYM, 1, 1);

	m_mutexStatus = pthread_mutex_init(&m_mtx, NULL);
	if( m_mutexStatus != 0 ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to initialize OCVBinaryNN mutex");
	}
}








OCVBinaryNN::~OCVBinaryNN(void)
{
	if( m_mutexStatus == 0 ) {
		pthread_mutex_destroy(&m_mtx);
	}
}




// bool OCVBinaryNN::Train(float *trainSet, int *labelVec,
// 					  int numObjs, int numDims)
// {
// 	Mat		features(numObjs, numDims, CV_32F, trainSet);
// 			// labels(numObjs, 1, CV_32S, labelVec);
//
// 	Mat labels(numObjs, 2, CV_32F);
// 	labels = 0.0;
//
// 	for(int i = 0; i <numObjs ; i++) {
// 			if( labelVec[i] > 0 )
// 				labels.at<float>(i, 1) = 1.0;
// 			else
// 				labels.at<float>(i, 0) = 1.0;
// 	}
//
// 	// Calculate priors for the training set
// 	m_priors[0] = 0.0f;
// 	m_priors[1] = 0.0f;
// 	for(int i = 0; i < numObjs; i++) {
// 		if( labelVec[i] == -1 )
// 			m_priors[0] += 1.0f;
// 		else
// 			m_priors[1] += 1.0f;
// 	}
// 	m_priors[1] /= (float)numObjs;
// 	m_priors[0] /= (float)numObjs;
//
// 	// m_params.nactive_vars = floor(sqrtf(numDims));
// 	pthread_mutex_lock(&m_mtx);
// 	m_trained = m_NN.train(features, labels, Mat(), Mat(), m_params);
// 	pthread_mutex_unlock(&m_mtx);
// 	return m_trained;
// }


bool OCVBinaryNN::Train(float *trainSet, int *labelVec,
					  int numObjs, int numDims)
{
	Mat		features(numObjs, numDims, CV_32F, trainSet);
			// labels(numObjs, 1, CV_32S, labelVec);

	Mat labels(numObjs, 2, CV_32F);
	labels = 0.0;

	for(int i = 0; i <numObjs ; i++) {
			if( labelVec[i] > 0 )
				labels.at<float>(i, 1) = 1.0;
			else
				labels.at<float>(i, 0) = 1.0;
	}

	// Calculate priors for the training set
	m_priors[0] = 0.0f;
	m_priors[1] = 0.0f;
	for(int i = 0; i < numObjs; i++) {
		if( labelVec[i] == -1 )
			m_priors[0] += 1.0f;
		else
			m_priors[1] += 1.0f;
	}
	m_priors[1] /= (float)numObjs;
	m_priors[0] /= (float)numObjs;

	// m_params.nactive_vars = floor(sqrtf(numDims));
	pthread_mutex_lock(&m_mtx);
	m_trained = m_NN.train(features, labels, Mat(), Mat(), m_params);
	pthread_mutex_unlock(&m_mtx);
	return m_trained;
}




int OCVBinaryNN::Classify(float *obj, int numDims)
{
	int	result = 0;
	float response = 0.0f;

	if( m_trained ) {
		Mat 	sample(1, numDims, CV_32F, obj);
		Mat responseMat(1, 2, CV_32F);

		pthread_mutex_lock(&m_mtx);
		m_NN.predict(sample, responseMat);
		pthread_mutex_unlock(&m_mtx);

		//result = responseMat.at<float>(0, 0) > 0 ? 1 : -1;

		float max = -1000000000000.0f;
		int cls = -1;
		for(int j = 0 ; j < 2 ; j++) {
				float value = responseMat.at<float>(0,j);
				if(value > max) {
						max = value;
						cls = j;
				}
		}
		result = (cls == 1) ? 1 : -1;

	}

	return result;
}






bool OCVBinaryNN::ClassifyBatch(float **dataset, int numObjs,
								  int numDims, int *results)
{
	bool	result = false;

	if( m_trained && results != NULL ) {

		vector<std::thread> workers;
		unsigned	numThreads = thread::hardware_concurrency();
		int			offset, objCount, remain, objsPer;

		remain = numObjs % numThreads;
		objsPer = numObjs / numThreads;

		pthread_mutex_lock(&m_mtx);

		// numThreads - 1 because main thread (current) will process also
		//
		for(int i = 0; i < numThreads - 1; i++) {

			objCount = objsPer + ((i <  remain) ? 1 : 0);
			offset = (i < remain) ? i * (objsPer + 1) :
						(remain * (objsPer + 1)) + ((i - remain) * objsPer);

			workers.push_back(std::thread(&OCVBinaryNN::ClassifyWorker, this,
							  std::ref(dataset), offset, objCount, numDims, results));
		}
		// Main thread's workload
		//
		objCount = objsPer;
		offset = (remain * (objsPer + 1)) + ((numThreads - remain - 1) * objsPer);

		ClassifyWorker(dataset, offset, objCount, numDims, results);

		for( auto &t : workers ) {
			t.join();
		}
		pthread_mutex_unlock(&m_mtx);
		result = true;
	}
	return result;
}




void OCVBinaryNN::ClassifyWorker(float **data, int offset, int numObjs, int numDims, int *results)
{

	if( m_trained && results != NULL ) {
		for(int i = offset; i < (offset + numObjs); i++) {
			Mat	object(1, numDims, CV_32F, data[i]);
			Mat responseMat(1, 2, CV_32F);

			m_NN.predict(object, responseMat);

			// results[i] = responseMat.at<float>(0, 0) > 0 ? 1 : -1;

			float max = -1000000000000.0f;
      int cls = -1;
      for(int j = 0 ; j < 2 ; j++) {
          float value = responseMat.at<float>(0, j);
          if(value > max) {
              max = value;
              cls = j;
          }
      }
			results[i] = (cls == 1) ? 1 : -1;
 		}
	}
}






float OCVBinaryNN::Score(float *obj, int numDims)
{
	float	score = 0.0;
	float max_score = 1.7159f;

	if( m_trained ) {
		Mat sample(1, numDims, CV_32F, obj);
		Mat responseMat(1, 2, CV_32F);

		pthread_mutex_lock(&m_mtx);
		m_NN.predict(sample, responseMat);
		pthread_mutex_unlock(&m_mtx);

		score = responseMat.at<float>(0, 1) / max_score;

		// // Returned a probability, center 50% at 0 and set range to -1 to 1
		// // score = (max * 2.0f) - 1.0f;
		// score = responseMat.at<float>(0, 1);

	}
	return score;
}





bool OCVBinaryNN::ScoreBatch(float **dataset, int numObjs,
							int numDims, float *scores)
{
	bool	result = false;

	if( m_trained && scores != NULL ) {
		vector<std::thread> workers;
		unsigned	numThreads = thread::hardware_concurrency();
		int			offset, objCount, remain, objsPer;

		remain = numObjs % numThreads;
		objsPer = numObjs / numThreads;

		pthread_mutex_lock(&m_mtx);
		// numThreads - 1 because main thread (current) will process also
		//
		for(int i = 0; i < numThreads - 1; i++) {

			objCount = objsPer + ((i <  remain) ? 1 : 0);
			offset = (i < remain) ? i * (objsPer + 1) :
						(remain * (objsPer + 1)) + ((i - remain) * objsPer);

			workers.push_back(std::thread(&OCVBinaryNN::ScoreWorker, this,
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
		pthread_mutex_unlock(&m_mtx);
		result = true;
	}
	return result;
}




void OCVBinaryNN::ScoreWorker(float **data, int offset, int numObjs, int numDims, float *results)
{
	float max_score = 1.7159f;
	int index = 0;
	if( m_trained && results != NULL ) {
		for(int i = offset; i < (offset + numObjs); i++) {
			Mat	object(1, numDims, CV_32F, data[i]);
			Mat responseMat(1, 2, CV_32F);
			// Mat responseMat(1, 1, CV_32FC1);

			m_NN.predict(object, responseMat);

			float max = -1000000000000.0f;
      int cls = -1;
      for(int j = 0 ; j < 2 ; j++) {
          float value = responseMat.at<float>(0, j);
          if(value > max) {
              max = value;
              cls = j;
          }
      }
			// results[i] = (cls == 1) ? 1 : -1;

			// results[i] = responseMat.at<float>(0, 1) / max_score;
			if (cls == 1)
				results[i] = responseMat.at<float>(0, 1) / max_score;
			else
				// results[i] = (responseMat.at<float>(0, 0) / max_score) - 1.0f;
				results[i] = (1.0f - (((responseMat.at<float>(0, 0) / max_score) + 1.0f) / 2.0f))*2.0f - 1.0f;
			// max = max / max_score;

			// Returned a probability, center 50% at 0 and set range to -1 to 1. This way
			// any object with a negative score is in the negative class and a positive
			// score indicates the positive class.
			//
			// results[i] = (max * 2.0f) - 1.0f;
		}
	}
}
