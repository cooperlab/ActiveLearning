#include <thread>

#include "ocvsvm.h"



using namespace std;



const float	OCVBinarySVM::SVM_C = 1;
const float	OCVBinarySVM::EPSILON = 1e-3;
const float	OCVBinarySVM::GAMMA = 0.5;



OCVBinarySVM::OCVBinarySVM(void)
{
	m_params.svm_type = CvSVM::C_SVC;
	m_params.kernel_type = CvSVM::RBF;
	m_params.gamma = GAMMA;
	m_params.C = SVM_C;
	m_params.term_crit = cvTermCriteria(CV_TERMCRIT_EPS, 100, EPSILON);
	m_params.coef0 = 0;
	m_params.nu = 0;
	m_params.p = 0.1f;
}




OCVBinarySVM::~OCVBinarySVM(void)
{


}







bool OCVBinarySVM::Train(float *&trainSet, int *labelVec,
					  int numObjs, int numDims)
{
	Mat		features(numObjs, numDims, CV_32F, trainSet),
			labels(numObjs, 1, CV_32S, labelVec);

	m_params.gamma = 1.0f / (float)numDims;
	m_trained = m_svm.train(features, labels, Mat(), Mat(), m_params);
	return m_trained;
}







int OCVBinarySVM::Classify(float *obj, int numDims)
{
	int	result = 0;

	if( m_trained ) {
		Mat 	sample(1, numDims, CV_32F, obj);

		result = (int)m_svm.predict(sample);
	}
	return result;
}






bool OCVBinarySVM::ClassifyBatch(float *&dataset, int numObjs,
								  int numDims, int *results)
{
	bool	result = false;

	if( m_trained && results != NULL ) {
		Mat	data(numObjs, numDims, CV_32F, dataset);

		for(int i = 0; i < numObjs; i++) {
			results[i] = (int)m_svm.predict(data.row(i));
		}
		result = true;
	}
	return result;
}





float OCVBinarySVM::Score(float *obj, int numDims)
{
	float	score = 0.0;

	if( m_trained ) {
		Mat 	sample(1, numDims, CV_32F, obj);

		// liopencv seems to return a negated score, compensate appropriately
		score = -m_svm.predict(sample, true);
	}
	return score;
}






bool OCVBinarySVM::ScoreBatch(float *dataset, int numObjs,
							int numDims, float *scores)
{
	bool	result = false;

	if( m_trained && scores != NULL ) {
		Mat	data(numObjs, numDims, CV_32F, dataset);
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

			workers.push_back(std::thread(&OCVBinarySVM::ScoreWorker, this,
							  std::ref(data), offset, objCount, numDims, scores));
		}

		// Main thread's workload
		//
		objCount = objsPer;
		offset = (remain * (objsPer + 1)) + ((numThreads - remain - 1) * objsPer);

		ScoreWorker(data, offset, objCount, numDims, scores);

		for( auto &t : workers ) {
			t.join();
		}
		result = true;
	}
	return result;
}






void OCVBinarySVM::ScoreWorker(Mat& data, int offset, int numObjs, int numDims, float *results)
{
	if( m_trained && results != NULL ) {

		for(int i = offset; i < (offset + numObjs); i++) {

			// liopencv seems to return a negated score, compensate appropriately
			//
			results[i] = -m_svm.predict(data.row(i), true);
		}
	}
}

