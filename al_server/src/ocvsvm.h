#if !defined(_SHOCLASSIFIER_H_)
#define _SHOCLASSIFIER_H_

#include <opencv2/core/core.hpp>
#include <opencv2/ml/ml.hpp>

#include "classifier.h"



using namespace cv;




class OCVBinarySVM : public Classifier
{
public:

					OCVBinarySVM(void);
	virtual			~OCVBinarySVM(void);


	virtual int 	Classify(float *obj, int numDims);
	virtual bool	ClassifyBatch(float *&dataset, int numObjs,
								  int numDims, int *results);

	virtual bool	Train(float *&trainSet, int *labelVec,
						  int numObjs, int numDims);

	virtual float	Score(float *obj, int numDims);
	virtual bool	ScoreBatch(float *dataset, int numObjs,
								int numDims, float *scores);


protected:

	static const float		SVM_C;
	static const float		EPSILON;
	static const float		GAMMA;


	CvSVMParams		m_params;
	CvSVM			m_svm;
};




#endif
