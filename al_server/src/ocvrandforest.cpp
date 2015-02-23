#include "ocvrandforest.h"






OCVBinaryRF::OCVBinaryRF(void)
{
	m_priors[0] = 1.0f;		// Each class is of equal importance
	m_priors[1] = 1.0f;

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






bool OCVBinaryRF::Train(float *&trainSet, int *labelVec,
					  int numObjs, int numDims)
{

	Mat		features(numObjs, numDims, CV_32F, trainSet),
			labels(numObjs, 1, CV_32S, labelVec);

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






bool OCVBinaryRF::ClassifyBatch(float *&dataset, int numObjs,
								  int numDims, int *results)
{
	bool	result = false;

	if( m_trained && results != NULL ) {
		Mat	data(numObjs, numDims, CV_32F, dataset);

		for(int i = 0; i < numObjs; i++) {
			results[i] = (int)m_RF.predict(data.row(i));
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





bool OCVBinaryRF::ScoreBatch(float *dataset, int numObjs,
							int numDims, float *scores)
{
	bool	result = false;

	if( m_trained && scores != NULL ) {
		Mat	data(numObjs, numDims, CV_32F, dataset);

		for(int i = 0; i < numObjs; i++) {
			scores[i] = m_RF.predict_prob(data.row(i));

			// Returned a probability, center 50% at 0 and set range to -1 to 1
			scores[i] = (scores[i] * 2.0f) - 1.0f;
		}
		result = true;
	}

	return result;
}

