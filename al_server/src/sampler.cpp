#include <cstdlib>
#include <string>
#include <vector>
#include <set>
#include <cstring>
#include <cfloat>
#include <ctime>
#include <cmath>
#include <algorithm>

#include "sampler.h"

#include "logger.h"


extern EvtLogger *gLogger;



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






// Select the 'count' most uncertain samples in the remaining set. The id's
// and score buffers are allcated here but must be freed by the caller.
//
bool UncertainSample::SelectBatch(int count, int *&ids, float *&selScores)
{
	bool	result = true;
	float	*checkSet = CreateCheckSet(),
			*scores = (float*)malloc(m_remaining * sizeof(float));
	int		*picks = (int*)malloc(count * sizeof(int));

	ids = (int*)malloc(count * sizeof(int));
	selScores = (float*)malloc(count * sizeof(float));

	if( ids == NULL || selScores == NULL || scores == NULL || picks == NULL ) {
		result = false;
	}

	if( result ) {
		checkSet = CreateCheckSet();
		if( !m_Classify->ScoreBatch(checkSet, m_remaining, m_dataset->GetDims(), scores) ) {
			result = false;
		}
	}

	if( result ) {
		int minIdx;
		float  min = FLT_MAX, objScore;

		for(int i = 0; i < count; i++) {
			selScores[i] = FLT_MAX;
		}

		for(int i = 0; i < m_remaining; i++) {
			objScore = abs(scores[i]);
			minIdx = count - 1;

			if( objScore < abs(selScores[minIdx]) ) {
				// Score is less than at least the last element, put it
				// there to start

				while( minIdx > 0 && objScore < abs(selScores[minIdx - 1]) ) {
					selScores[minIdx] = selScores[minIdx - 1];
					picks[minIdx] = picks[minIdx - 1];
					minIdx--;
				}
				selScores[minIdx] = scores[i];
				picks[minIdx] = i;
			}
		}

		// Check if the uncertainty score varies... If not we may need to "spread out"
		// the object selection among the available slides.
		//
		if( selScores[0] == selScores[count - 1] ) {
			vector<int>	minObjs;
			set<int>	repSlides;
			int			*slideIdx = m_dataset->GetSlideIndices();

			// All scores have the same uncertainty. Get all objects with a
			// score equal to the min and TODO select objects from as many slides
			// as possible. Note - For now we are just selecting randomly from
			// the objects with the min value.
			//
			for(int i = 0; i < m_remaining; i++) {
				objScore = abs(scores[i]);
				if( objScore == selScores[0] ) {
					minObjs.push_back(i);
					repSlides.insert(slideIdx[m_dataIndex[i]]);
				}
			}

			gLogger->LogMsgv(EvtLogger::Evt_INFO, "Uncertain obj count: %d", minObjs.size());
			minIdx = 0;
			for(int i = 0; i < count; i++ ) {
				int obj = rand() % minObjs.size();
				picks[minIdx++] = minObjs[obj];
				minObjs.erase(minObjs.begin() + obj);
			}
		}

		// Remove selected objects from remaining set
		m_remaining--;
		for(int i = 0; i < count; i++) {
			ids[i] = m_dataIndex[picks[i]];
			m_dataIndex[picks[i]] = m_dataIndex[m_remaining];
			m_remaining--;
		}
	}

	if( picks )
		free(picks);
	if( scores )
		free(scores);
	if( checkSet ) 
		free(checkSet);

	return result;
}




// TODO - Create checkset once, then remove the objects that were added to
//	the training set. More efficient than creating the checkset every time


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




struct ScoreIdx{
	int	idx;
	float score;
};

bool SortFunc(ScoreIdx a, ScoreIdx b)
{
	return (a.score < b.score);
}




//
//	Select samples for visualization. nStrata specifies the number of uncertainty
// 	levels. An nStrata of 1 will select the most uncertain sample, 2 will select
//  the most uncertain and the most certain, 3 will add the 50% percentile and
//  so on. nGroups specifies the number of samples per strata. Samples will be
//	returned for both clases, so for an nStrata of 4 and nGroups of 2 there will
//	be 4(strata) * 2(groups) * 2(classes) samples for a total of 16 returned.
//	They are ordered as follows: Group 1 Neg class most certain to most uncertain,
//	Group 1 Pos class most uncertain to most certain, ... Group N Neg class most
//	certain to most uncertain, Group N Pos class most uncertain to most certain.
//
//
bool UncertainSample::GetVisSamples(int nStrata, int nGroups, int *&idx, float *&idxScores)
{
	bool	result = true;
	float	*checkSet = CreateCheckSet(),
			*scores = (float*) malloc(m_remaining * sizeof(float));


	idx = (int*)calloc(nStrata * nGroups * 2, sizeof(int));
	idxScores = (float*)calloc(nStrata * nGroups * 2, sizeof(float));

	// Score the unlabeled data and split into 2 classes
	if( scores && idxScores && idx && checkSet ) {

		vector<ScoreIdx> neg, pos;
		ScoreIdx	temp;

		if( m_Classify->ScoreBatch(checkSet, m_remaining, m_dataset->GetDims(), scores) ) {

			for(int i = 0; i < m_remaining; i++) {
				if( scores[i] < 0 ) {
					temp.score = scores[i];
					temp.idx = m_dataIndex[i];
					neg.push_back(temp);
				} else {
					temp.score = scores[i];
					temp.idx = m_dataIndex[i];
					pos.push_back(temp);
				}
			}

			sort(neg.begin(), neg.end(), SortFunc);
			sort(pos.begin(), pos.end(), SortFunc);

			float posPercent[nStrata], negPercent[nStrata];

			posPercent[0] = negPercent[0] = 0;
			posPercent[nStrata - 1] = pos.size() - nGroups - 1;
			negPercent[nStrata - 1] = neg.size() - nGroups - 1;

			float stride = (1.0f / (float)(nStrata - 1));
			for(int i = 1; i < nStrata - 1; i++) {
				posPercent[i] = i * stride * pos.size();
				negPercent[i] = i * stride * neg.size();
			}

			for(int grp = 0; grp < nGroups; grp++) {
				for(int s = 0; s < nStrata; s++) {
					idx[(grp * 2 * nStrata) + s] = neg[negPercent[s] + grp].idx;
					idx[(grp * 2 * nStrata) + s + nStrata] = pos[posPercent[s] + grp].idx;

					if( idxScores ) {
						idxScores[(grp * 2 * nStrata) + s] = neg[negPercent[s] + grp].score;
						idxScores[(grp * 2 * nStrata) + s + nStrata] = pos[posPercent[s] + grp].score;
					}
				}
			}
		}
		if( checkSet )
			free(checkSet);
	}

	if( scores )
		free(scores);

	return result;
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







bool RandomSample::SelectBatch(int count, int *&ids, float *&scores)
{
	bool	result = false;

	// TODO - Need to implement

	return result;
}


