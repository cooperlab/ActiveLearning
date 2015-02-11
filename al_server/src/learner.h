#if !defined(_LEARNER_H_)
#define _LEARNER_H_

#include <vector>
#include <string>
#include <set>
#include <jansson.h>

#include "data.h"
#include "ocvsvm.h"
#include "sampler.h"


#define UID_LENGTH 		23




class Learner
{
public:
			Learner(string dataPath = "./", string outPath = "./");
			~Learner(void);

	bool	ParseCommand(const int sock, char *data, int size);


protected:

	char	m_UID[UID_LENGTH + 1];
	MData	*m_dataset;
	MData	*m_classTrain;	// Used for applying classifier when no session active
	string	m_dataPath;
	string 	m_outPath;
	string	m_classifierName;
	string	m_curDatasetName;

	vector<int> m_samples;
	vector<int>	m_curSet;
	vector<float> m_curScores;

	// Training set info
	//
	int			*m_labels;
	int			*m_ids;
	float		*m_trainSet;
	int			*m_sampleIter;	// Iteration sample was added
	int			*m_slideIdx;
	float		*m_xCentroid;
	float		*m_yCentroid;

	int			m_iteration;
	float		m_curAccuracy;

	Classifier 	*m_classifier;
	Sampler		*m_sampler;

	bool		m_pickerMode;


	bool	StartSession(const int sock, json_t *obj);
	bool	Select(const int sock, json_t *obj);
	bool	Submit(const int sock, json_t *obj);
	bool	CancelSession(const int sock, json_t *obj);
	bool	FinalizeSession(const int sock, json_t *obj);
	bool	ApplyClassifier(const int sock, json_t *obj);
	bool	Visualize(const int sock, json_t *obj);
	bool	InitPicker(const int sock, json_t *obj);
	bool	AddObjects(const int sock, json_t *obj);
	bool	PickerStatus(const int sock, json_t *obj);
	bool	PickerFinalize(const int sock, json_t *obj);

	bool	ApplyGeneralClassifier(const int sock, json_t *obj);
	bool	ApplySessionClassifier(const int sock, json_t *obj);
	bool	SendClassifyResult(int xMin, int xMax, int yMin, int yMax,
			 	 	 	 	   string slide, int *results, const int sock);

	bool 	UpdateBuffers(int updateSize);
	void	Cleanup(void);

	bool	SaveTrainingSet(string filename);

	bool	InitSampler(void);

	float	CalcAccuracy(void);
	bool	CreateSet(vector<int> folds, int fold, float *&trainX,
					  int *&trainY, float *&testX, int *&testY);

	bool	LoadDataset(string dataSetFileName);
	bool	LoadTrainingSet(string trainingSetName);


};




#endif
