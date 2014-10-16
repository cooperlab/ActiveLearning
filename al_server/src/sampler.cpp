#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cfloat>
#include <ctime>
#include <cmath>


#include "sampler.h"




Sampler::Sampler(MData *dataset) :
m_dataIndex(NULL),
m_remaining(0),
m_dataset(dataset)
{
	srand(time(NULL));
}





Sampler::~Sampler(void)
{
	if( m_dataIndex )
		free(m_dataIndex);
}



//
//	Let the sampler know which samples were selected
// 	by the user during priming.
//
bool Sampler::Init(int count, int *list)
{
	bool	result = false;
	int		pick;

	for(int i = 0; i < count; i++) {
		for(int j = 0; j < m_remaining; j++) {
			if( m_dataIndex[j] == list[i] ) {
				pick = j;
				break;
			}
		}
		m_remaining--;
		m_dataIndex[pick] = m_dataIndex[m_remaining];
	}
	return result;
}





//-----------------------------------------------------------------------------


UncertainSample::UncertainSample(Classifier *classify, MData *dataset) : Sampler(dataset),
m_Classify(classify)
{
	m_dataIndex = (int*)malloc(dataset->GetNumObjs() * sizeof(int));

	if( m_dataIndex ) {
		m_remaining = dataset->GetNumObjs();
		for(int i = 0; i < m_remaining; i++)
			m_dataIndex[i] = i;
	}

}




UncertainSample::~UncertainSample(void)
{

}





// Select the most uncertain object in the remaining set. The score returned by
// ScoreBatch is the conditional entropy for the object. The object with the
// maximum conditional entropy is the most uncertain.
//
int UncertainSample::Select(float *score)
{
	int		pick = -1;
	float	*checkSet = CreateCheckSet(),
			*scores = (float*) malloc(m_remaining * sizeof(float));

	if( scores ) {
		if( m_Classify->ScoreBatch(checkSet, m_remaining, m_dataset->GetDims(), scores) ) {
			int minIdx = -1;
			float  min = FLT_MAX;

			for(int i = 0; i < m_remaining; i++) {

				if( abs(scores[i]) < min ) {
					min = abs(scores[i]);
					minIdx = i;
				}
			}

			if( score != NULL )
				*score = scores[minIdx];

			pick = m_dataIndex[minIdx];
			m_remaining--;
			m_dataIndex[minIdx] = m_dataIndex[m_remaining];
		}
		if( checkSet )
			free(checkSet);
		free(scores);
	}
	return pick;
}







float* UncertainSample::CreateCheckSet(void)
{
	int		dims = m_dataset->GetDims();
	float	*checkSet = NULL;

	checkSet = (float*)malloc(m_remaining * dims * sizeof(float));
	if( checkSet ) {
		float	**data = m_dataset->GetData();

		for(int i = 0; i < m_remaining; i++) {
			memcpy(&checkSet[i * dims], data[m_dataIndex[i]], dims * sizeof(float));
		}
	}
	return checkSet;
}






//------------------------------------------------------------------------------


RandomSample::RandomSample(MData *dataset) : Sampler(dataset)
{

	int numObjs = dataset->GetNumObjs();
	m_dataIndex = (int*)malloc(numObjs * sizeof(int));
	if( m_dataIndex ) {
		m_remaining = numObjs;
		for(int i = 0; i < numObjs; i++)
			m_dataIndex[i] = i;
	}
}



RandomSample::~RandomSample(void)
{

}






// Selects a random object without replacement.
// Returns -1 if no objects are left
//
int RandomSample::Select(float *score)
{
	int	obj = -1, pick;

	if( m_dataIndex && m_remaining > 0 ) {
		pick = rand() % m_remaining;
		obj = m_dataIndex[pick];

		// Put last obj of list in spot just selected
		m_remaining--;
		m_dataIndex[pick] = m_dataIndex[m_remaining];
	}
	return obj;
}




