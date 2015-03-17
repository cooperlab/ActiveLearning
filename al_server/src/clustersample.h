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
