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
#if !defined(_LEARNER_H_)
#define _LEARNER_H_

#include <vector>
#include <string>
#include <set>
#include <jansson.h>

#include "data.h"
#include "ocvsvm.h"
#include "ocvrandforest.h"
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

	set<int>	m_ignoreSet;	// Contains the dataset index of objects to ignore

	// Training set info
	//
	int			*m_labels;
	int			*m_ids;
	float		**m_trainSet;
	int			*m_sampleIter;	// Iteration sample was added
	int			*m_slideIdx;
	float		*m_xCentroid;
	float		*m_yCentroid;

	int			m_iteration;
	float		m_curAccuracy;

	Classifier 	*m_classifier;
	Sampler		*m_sampler;

	bool		m_pickerMode;

	bool		m_debugStarted;

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

	bool	DebugClassify(const int sock, json_t *obj);
	bool	DebugApply(ofstream& outFile, int iteratioin);

	bool	ApplyGeneralClassifier(const int sock, int xMin, int xMax,
								   int yMin, int yMax, string slide);
	bool	ApplySessionClassifier(const int sock, int xMin, int xMax,
								   int yMin, int yMax, string slide);
	bool	SendClassifyResult(int xMin, int xMax, int yMin, int yMax,
			 	 	 	 	   string slide, int *results, const int sock);

	bool	InitViewerClassify(const int sock, json_t *obj);

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
