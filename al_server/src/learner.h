#if !defined(_LEARNER_H_)
#define _LEARNER_H_

#include <vector>
#include <set>
#include <jansson.h>

#include "data.h"
#include "ocvsvm.h"
#include "sampler.h"


#define UID_LENGTH 		23




class Learner
{
public:
			Learner(string dataPath = "./");
			~Learner(void);

	bool	ParseCommand(const int sock, char *data, int size);


protected:

	char	m_UID[UID_LENGTH + 1];
	MData	*m_dataset;
	string	m_dataPath;
	string	m_classifierName;

	vector<int> m_samples;
	vector<int>	m_curSet;
	vector<float> m_curScores;


	int			*m_labels;
	int			*m_ids;
	float		*m_trainSet;
	int			*m_sampleIter;	// Iteration sample was added
	int			m_iteration;
	float		m_curAccuracy;

	Classifier 	*m_classifier;
	Sampler		*m_sampler;

	bool	StartSession(const int sock, json_t *obj);
	bool	Select(const int sock, json_t *obj);
	bool	Submit(const int sock, json_t *obj);
	bool	CancelSession(const int sock, json_t *obj);
	bool	FinalizeSession(const int sock, json_t *obj);
	bool	ApplyClassifier(const int sock, json_t *obj);

	bool 	UpdateBuffers(int updateSize);
	void	Cleanup(void);

	bool	SaveTrainingSet(string filename);

	bool	InitSampler(void);

	float	CalcAccuracy(void);
	bool	CreateSet(vector<int> folds, int fold, float *&trainX,
					  int *&trainY, float *&testX, int *&testY);

};




#endif
