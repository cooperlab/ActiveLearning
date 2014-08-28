#if !defined(_CLASSIFIER_H_)
#define _CLASSIFIER_H_



class Classifier
{
public:
					Classifier();
	virtual			~Classifier();

	virtual bool 	Train(float *&trainSet, int *labelSet,
						  int numObjs, int numDims) = 0;

	virtual int 	Classify(float *obj, int numDims) = 0;
	virtual bool	ClassifyBatch(float *&dataset, int numObjs,
								  int numDims, int *results) = 0;

	virtual float	Score(float *obj, int numDims) = 0;
	virtual bool	ScoreBatch(float *dataset, int numObjs,
								int numDims, float *scores) = 0;

protected:

	bool			m_trained;
};






#endif
