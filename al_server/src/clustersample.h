#if !defined(_CLUSTERSAMPLE_H_)
#define _CLUSTERSAMPLE_H_

#include <set>
#include "sampler.h"


//
// Defined in  kmeans.cpp
//
float** kmeans(float **objects,      /* in: [numObjs][numCoords] */
               int     numCoords,    /* no. features */
               int     numObjs,      /* no. objects */
               int     numClusters,  /* no. clusters */
               float   threshold,    /* % objects change membership */
               int    *membership);   /* out: [numObjs] */

float euclid_dist_2(int    numdims,  /* no. dimensions */
              				float *coord1,   /* [numdims] */
               				float *coord2);   /* [numdims] */

//-----------------------------------------------------------------------------



class ClusterSample : public UncertainSample
{
public:

			ClusterSample(Classifier *classify, MData *dataset, int k = 50);
			~ClusterSample(void);

	virtual bool	Init(int count, int *list);
	virtual int		Select(void);
	virtual int		Select(int& other);
	virtual void	Reset(void);

	virtual SampType 	GetSamplerType(void) { return SAMP_CLUSTER; }


	void	GetSupportSet(set<int>& supportSet, int *&supportLabels);
	bool	SetSupportSet(int *objs, int count);

protected:

	struct	LabelCnt {
		int		pos;
		int		neg;
	};

	bool	m_initialized;
	int		*m_membership;
	int		m_k;

	set<int> *m_clusters;
	float	**m_centroids;
	float	*m_compactness;
	LabelCnt	*m_labelCnt;
	set<int>	*m_sampledClusters;

	bool	ClusterData(void);
	float*	CreateCheckSet(int cluster, int altCluster = -1);
	bool	CalcMetrics(void);

};




#endif
