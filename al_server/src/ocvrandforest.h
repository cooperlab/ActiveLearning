#if !defined(_OCVRANDFOREST_H_)
#define _OCVRANDFOREST_H_

#include <opencv2/core/core.hpp>
#include <opencv2/ml/ml.hpp>

#include "classifier.h"


using namespace cv;




class OCVBinaryRF : public Classifier
{
public:

				OCVBinaryRF(void);
	virtual		~OCVBinaryRF(void);


	virtual int 	Classify(float *obj, int numDims);
	virtual bool	ClassifyBatch(float *&dataset, int numObjs,
								  int numDims, int *results);

	virtual bool	Train(float *&trainSet, int *labelVec,
						  int numObjs, int numDims);

	virtual float	Score(float *obj, int numDims);
	virtual bool	ScoreBatch(float *dataset, int numObjs,
								int numDims, float *scores);


protected:

	float			m_priors[2];
	CvRTParams		m_params;
	CvRTrees		m_RF;


};


#endif
