#include <iostream>
#include <cfloat>
#include <cstring>
#include <cmath>
#include "clustersample.h"


// Threshold for k-means stopping point
#define CHANGE_THRESH  0.001f



ClusterSample::ClusterSample(Classifier *classify, MData *dataset, int k) :
			   UncertainSample(classify, dataset),
m_initialized(false),
m_k(k),
m_clusters(NULL),
m_centroids(NULL),
m_compactness(NULL),
m_labelCnt(NULL),
m_sampledClusters(NULL)
{
	m_membership = (int*)malloc(dataset->GetNumObjs() * sizeof(int));

	if( m_membership ) {
		m_initialized = ClusterData();
	} else {
		cerr << "Unable to allocate membership buffer" << endl;
	}
}






ClusterSample::~ClusterSample(void)
{
	if( m_membership )
		free(m_membership);

	if( m_clusters )
		delete [] m_clusters;

	if( m_centroids ) {
		if( m_centroids[0] )
			free(m_centroids[0]);
		free(m_centroids);
	}

	if( m_compactness )
		free(m_compactness);

	if( m_labelCnt )
		free(m_labelCnt);

	if( m_sampledClusters )
   		delete m_sampledClusters;
}






// Call the parent object method first, then remove the
// selected items from the cluster sets.
//
bool ClusterSample::Init(int count, int *list)
{
	bool	result = false;
	int		*labels = m_dataset->GetLabels();

	m_sampledClusters = new set<int>;

	if( m_initialized && m_sampledClusters ) {
		result = UncertainSample::Init(count, list);

		if( result ) {
			for(int i = 0; i < count; i++) {

				m_clusters[m_membership[list[i]]].erase(list[i]);

				if( labels[list[i]] == -1 )
					m_labelCnt[m_membership[list[i]]].neg++;
				else
					m_labelCnt[m_membership[list[i]]].pos++;
				m_sampledClusters->insert(m_membership[list[i]]);
			}
		}
	}
	return result;
}





//
//	Re-populate the cluster sets to allow a new round
//  of sampling.
//
void ClusterSample::Reset(void)
{
	UncertainSample::Reset();

	// Recluster data
	if( m_centroids ) {
		if( m_centroids[0] )
			free(m_centroids[0]);
		free(m_centroids);
		m_centroids = NULL;
	}

	m_initialized = ClusterData();

	// Clear all cluster sets
	for(int i = 0; i < m_k; i++)
		m_clusters[i].clear();

	// The Init function will recreate the sampled clusters object.
	delete m_sampledClusters;
	m_sampledClusters = NULL;

	// Put all data back into the clusters
	for(int i = 0; i < m_dataset->GetNumObjs(); i++) {
		m_clusters[m_membership[i]].insert(i);
	}
}





bool ClusterSample::SetSupportSet(int *objs, int count)
{
	bool	result = false;

	m_sampledClusters = new set<int>;
	if( m_sampledClusters ) {
		for(int i = 0; i < count; i++)
			m_sampledClusters->insert(m_membership[objs[i]]);
		result = true;
	}
	return result;
}






void ClusterSample::GetSupportSet(set<int>& supportSet, int *&supportLabels)
{
	int 	*newLabels = NULL, 	dims = m_dataset->GetDims();

	if( supportLabels != NULL ) {
		free(supportLabels);
		supportLabels = NULL;
	}

	set<int>::iterator it, objIt;

	for(it = m_sampledClusters->begin(); it != m_sampledClusters->end(); it++) {
		bool	select = false;
		int 	majority = (m_labelCnt[*it].neg <  m_labelCnt[*it].pos) ? 1 : -1;
		float 	*clustObjs = CreateCheckSet(*it),
				*objScores = (float*)malloc(m_clusters[*it].size() * sizeof(float));

		m_Classify->ScoreBatch(clustObjs, m_clusters[*it].size(), m_dataset->GetDims(), objScores);

		for(int i = 0; i < m_clusters[*it].size(); i++) {
			if( objScores[i] * majority >= 0 ) {
				select = true;
			} else
				break;
		}

		if( select ) {
			int newSize = m_clusters[*it].size() + supportSet.size();
			newLabels = (int*)realloc(supportLabels, newSize * dims * sizeof(float));

			if( newLabels )
				supportLabels = newLabels;
			else  {
				cerr << "Unable to adjust buffer" << endl;
				exit(-1);
			}
			int		idx = supportSet.size();
			for(objIt = m_clusters[*it].begin(); objIt != m_clusters[*it].end(); objIt++) {
				supportSet.insert(*objIt);
				supportLabels[idx++] = majority;
			}
		}
		free(clustObjs);
		free(objScores);
	}
}






int ClusterSample::Select(void)
{
	int	 obj = -1, objMinIdx = 0;
	float *clustScores = (float*) malloc(m_k * sizeof(float)),
		  *testScores = (float*)malloc(m_dataset->GetNumObjs() * sizeof(float)),
		  objMin = FLT_MAX;	int	*testLabels = (int*)malloc(m_dataset->GetNumObjs() * sizeof(int));
	int *labels = m_dataset->GetLabels();


	if( clustScores && testScores ) {
		float	**data = m_dataset->GetData();

		for(int i = 0; i < m_k; i++) {
			if( m_clusters[i].size() > 0 )
				clustScores[i] = m_Classify->Score(m_centroids[i], m_dataset->GetDims());
			else
				clustScores[i] = FLT_MAX;
		}

		m_Classify->ScoreBatch(data[0], m_dataset->GetNumObjs(), m_dataset->GetDims(), testScores);

		set<int>::iterator it;
#if 0
		cout << "Obj uncert: ";
		for(int i = 0; i < m_remaining; i++) {
			if( abs(testScores[m_dataIndex[i]]) < objMin ) {
				objMin = abs(testScores[m_dataIndex[i]]);
				objMinIdx = i;
			}
			cout << m_dataIndex[i] << ": " << testScores[m_dataIndex[i]] << ", ";
		}
		cout << endl << endl;

		cout << "Clust uncert: ";
		for(int i = 0; i < m_k; i++) {
			cout << i << ": " << clustScores[i] << ", ";
		}
		cout << endl << endl;

		for(int c = 0; c < m_k; c++) {
			float 	*clustObjs = CreateCheckSet(c),
					*objScores = (float*)malloc(m_clusters[c].size() * sizeof(float));

			m_Classify->ScoreBatch(clustObjs, m_clusters[c].size(), m_dataset->GetDims(), objScores);

			if(  m_clusters[c].size() > 0 ) {
				cout << "Clust " << c << ": score: " << clustScores[c] << ", size: " << m_clusters[c].size()
					<< ", compactness: " << m_compactness[c] << ", pos count: " << m_labelCnt[c].pos
					<< ", neg count: " << m_labelCnt[c].neg << endl;
				cout << "Members: ";

				int i;
				for(it = m_clusters[c].begin(), i = 0; it != m_clusters[c].end(); it++, i++) {
					printf("%d: score: %2.6f (%d) ", *it, objScores[i], labels[*it]);
				}
			} else {
				cout << "Clust " << c << ": Empty";
			}
			cout << endl << endl;

			free(objScores);
			free(clustObjs);
		}
#endif

		// Find cluster closest to the hyperplane... Make sure the cluster's not empty
		float 	minScore = FLT_MAX;
		int		minIdx = -1, minObj = -1;
		bool	empty = true;

		for(int i = 0; i < m_k; i++) {

			if( m_clusters[i].size() > 0 && abs(clustScores[i]) < minScore ) {
				empty = false;
				minIdx = i;
				minScore = abs(clustScores[i]);
			}
		}

		if( !empty ) {
			// Score the objects from the cluster
			float 	*clustObjs = CreateCheckSet(minIdx),
					*objScores = (float*)malloc(m_clusters[minIdx].size() * sizeof(float));
			int *clustObjLabels = (int*)malloc(m_clusters[minIdx].size() * sizeof(int));

			m_sampledClusters->insert(minIdx);

			if( clustObjs && objScores && clustObjLabels ) {
				m_Classify->ScoreBatch(clustObjs, m_clusters[minIdx].size(), m_dataset->GetDims(), objScores);
				m_Classify->ClassifyBatch(clustObjs, m_clusters[minIdx].size(), m_dataset->GetDims(), clustObjLabels);
				free(clustObjs);
			}

			// Now find object closest to the hyperplane
			minScore = FLT_MAX;
			int	idx = 0;
			float newRadius = 0.0f;

			for(it = m_clusters[minIdx].begin(); it != m_clusters[minIdx].end(); it++) {

				if( abs(objScores[idx]) < minScore ) {
					minScore = abs(objScores[idx]);
					minObj = *it;
				}
				idx++;
			}

			// Remove the selected object from the cluster centroid
			int clustSize = m_clusters[minIdx].size(),
				dims = m_dataset->GetDims();

			for(int i = 0; i < dims; i++) {
				m_centroids[minIdx][i] *= clustSize;
				m_centroids[minIdx][i] -= data[minObj][i];
				m_centroids[minIdx][i] /= (clustSize - 1);
			}
			// Remove object from set
			m_clusters[minIdx].erase(minObj);
			obj = minObj;

			if( labels[minObj] == 1 )
				m_labelCnt[minIdx].pos++;
			else
				m_labelCnt[minIdx].neg++;

#if 0
			cout << "Selected: " << obj << "(" << minIdx << "), searched: " << idx + m_k
				 << ", min: " << objMin << " (" << objMinIdx << ") clust: " << m_membership[objMinIdx] << endl;
#endif
			int swap = 0;
			while( m_dataIndex[swap] != obj )
				swap++;

			m_remaining--;
			m_dataIndex[swap] = m_dataIndex[m_remaining];

			if( objScores )
				free(objScores);
		}
	}
	return obj;
}





#if 0


int ClusterSample::Select(int &other)
{
	int	 obj = -1, objMinIdx = 0;
	float *clustScores = (float*) malloc(m_k * sizeof(float)),
		  *testScores = (float*)malloc(m_dataset->GetNumObjs() * sizeof(float)),
		  objMin = FLT_MAX;	int	*testLabels = (int*)malloc(m_dataset->GetNumObjs() * sizeof(int));

	other = -1;

	if( clustScores && testScores ) {
		float	**data = m_dataset->GetData();

		for(int i = 0; i < m_k; i++) {
			if( m_clusters[i].size() > 0 )
				clustScores[i] = m_Classify->Score(m_centroids[i], m_dataset->GetDims());
			else
				clustScores[i] = FLT_MAX;
		}
		m_Classify->ScoreBatch(data[0], m_dataset->GetNumObjs(), m_dataset->GetDims(), testScores);

		set<int>::iterator	it;
#if 0
		cout << "Obj uncert: ";
		for(int i = 0; i < m_remaining; i++) {
			if( abs(testScores[m_dataIndex[i]]) < objMin ) {
				objMin = abs(testScores[m_dataIndex[i]]);
				objMinIdx = i;
			}
			cout << m_dataIndex[i] << ": " << testScores[m_dataIndex[i]] << ", ";
		}
		cout << endl << endl;

		cout << "Clust uncert: ";
		for(int i = 0; i < m_k; i++) {
			cout << i << ": " << clustScores[i] << ", ";
		}
		cout << endl << endl;
#if 0
		for(int c = 0; c < m_k; c++) {
			float 	*clustObjs = CreateCheckSet(c),
					*objScores = (float*)malloc(m_clusters[c].size() * sizeof(float));

			m_Classify->ScoreBatch(clustObjs, m_clusters[c].size(), m_dataset->GetDims(), objScores);

			if(  m_clusters[c].size() > 0 ) {
				cout << "Clust " << c << ": score: " << clustScores[c] << ", size: " << m_clusters[c].size()
						<< ", radius: " << m_radii[c] << ", Avg Dist: " << m_avgDist[c] << endl;
				cout << "Members: ";

				int i;
				for(it = m_clusters[c].begin(), i = 0; it != m_clusters[c].end(); it++, i++) {
					printf("%d: score: %2.6f, dist: %3.6f ", *it, objScores[i], m_objDist[*it]);
				}
			} else {
				cout << "Clust " << c << ": Empty";
			}
			cout << endl << endl;

			free(objScores);
			free(clustObjs);
		}
#endif
#endif


		// Find 2 clusters closest to the hyperplane... Make sure the cluster's not empty
		float 	minScore = FLT_MAX, nextMin = FLT_MAX, altScore = FLT_MAX;
		int		minIdx = -1, minObj = -1, nextMinIdx = -1, nextMinObj = -1, altIdx = -1, side;
		bool	empty = true;


		for(int i = 0; i < m_k; i++) {

			if( m_clusters[i].size() > 0 && abs(clustScores[i]) < minScore ) {
				empty = false;
				nextMinIdx = minIdx;
				nextMin = minScore;
				minIdx = i;
				minScore = abs(clustScores[i]);
				side = (clustScores[i] < 0) ? -1 : 1;

			} else if( m_clusters[i].size() > 0 && abs(clustScores[i]) < nextMin ) {
				empty = false;
				nextMinIdx = i;
				nextMin = abs(clustScores[i]);
			}
		}

		// minIdx is the cluster closest to the hyperplane. nextMinIdx in the next
		// closest cluster regardless of which side (+ / -) We will use nextMinIdx
		// if there's no cluster on the opposite side of the hyperplane from minIdx.
		for(int i = 0; i < m_k; i++) {
			if( m_clusters[i].size() > 0 ) {

				if( clustScores[i] * side < 0 ) {
					// Cluster is on opposite side of hyperplane
					if( abs(clustScores[i]) < altScore ) {
						altScore = abs(clustScores[i]);
						altIdx = i;
					}
				}
			}
		}

		//cout << "\033[1;35m" << "minIdx: " << minIdx << ", nextMinIdx: " << nextMinIdx << ", altIdx: " << altIdx
		//		<< "\033[0m" << endl;


		if( altIdx != -1 )
			nextMinIdx = altIdx;	// Found a cluster on the oppsoite side of the hyperplane

		if( !empty ) {

			// Score the objects from the cluster
			int 	size = m_clusters[minIdx].size(), dims = m_dataset->GetDims();
			if( altIdx != -1 )
				size += + m_clusters[altIdx].size();

			float 	*clustObjs = CreateCheckSet(minIdx, altIdx),
					*objScores = (float*)malloc(size * sizeof(float));
			int 	*clustObjLabels = (int*)malloc(size * sizeof(int));

			if( clustObjs && objScores && clustObjLabels ) {

				m_Classify->ScoreBatch(clustObjs, size, m_dataset->GetDims(), objScores);
				m_Classify->ClassifyBatch(clustObjs, size, m_dataset->GetDims(), clustObjLabels);
				free(clustObjs);
			}

			// Now find object closest to the hyperplane in each cluster
			minScore = FLT_MAX;
			int	idx = 0;
			float newRadius = 0.0f;

			for(it = m_clusters[minIdx].begin(); it != m_clusters[minIdx].end(); it++) {

				if( abs(objScores[idx]) < minScore ) {
					minScore = abs(objScores[idx]);
					minObj = *it;
				}
				idx++;
			}

			// Remove the selected object from the cluster centroid
			int clustSize = m_clusters[minIdx].size();

			for(int i = 0; i < dims; i++) {
				m_centroids[minIdx][i] *= clustSize;
				m_centroids[minIdx][i] -= data[minObj][i];
				m_centroids[minIdx][i] /= (clustSize - 1);
			}

			// Remove object from set
			m_clusters[minIdx].erase(minObj);
			obj = minObj;


			//
			// Second object
			//
			if( altIdx != -1 ) {
				nextMin = FLT_MAX;
				newRadius = 0.0f;

				for(it = m_clusters[altIdx].begin(); it != m_clusters[altIdx].end(); it++) {

					if( abs(objScores[idx]) < nextMin ) {
						nextMin = abs(objScores[idx]);
						nextMinObj = *it;
					}
					idx++;
				}

				// Remove the selected object from the cluster centroid
				clustSize = m_clusters[altIdx].size();

				for(int i = 0; i < dims; i++) {
					m_centroids[altIdx][i] *= clustSize;
					m_centroids[altIdx][i] -= data[altIdx][i];
					m_centroids[altIdx][i] /= (clustSize - 1);
				}

				// Remove object from set
				m_clusters[altIdx].erase(nextMinObj);
				other = nextMinObj;
			}
#if 0
			cout << "Selected: " << obj << " (" << minIdx << "),  " << other << " (" << nextMinIdx << ") searched: " << idx + m_k
					<< ", min: " << objMin << " (" << objMinIdx << ") clust: " << m_membership[objMinIdx] << endl;
#endif

			int swap = 0;
			while( m_dataIndex[swap] != obj )
				swap++;

			m_remaining--;
			m_dataIndex[swap] = m_dataIndex[m_remaining];

			if( altIdx != -1 ) {
				swap = 0;
				while( m_dataIndex[swap] != other )
					swap++;

				m_remaining--;
				m_dataIndex[swap] = m_dataIndex[m_remaining];
			}
			if( objScores )
				free(objScores);
			if( clustObjLabels )
				free(clustObjLabels);
		}
	}
	if( clustScores )
		free(clustScores);
	if( testScores )
		free(testScores);

	return obj;
}



#else




int ClusterSample::Select(int &other)
{
	int	 obj = -1, objMinIdx = 0;
	float *clustScores = (float*) malloc(m_k * sizeof(float)),
		  *testScores = (float*)malloc(m_dataset->GetNumObjs() * sizeof(float)),
		  objMin = FLT_MAX;	int	*testLabels = (int*)malloc(m_dataset->GetNumObjs() * sizeof(int));

	other = -1;

	if( clustScores && testScores ) {
		float	**data = m_dataset->GetData();

		for(int i = 0; i < m_k; i++) {
			if( m_clusters[i].size() > 0 )
				clustScores[i] = m_Classify->Score(m_centroids[i], m_dataset->GetDims());
			else
				clustScores[i] = FLT_MAX;
		}

		m_Classify->ScoreBatch(data[0], m_dataset->GetNumObjs(), m_dataset->GetDims(), testScores);

		set<int>::iterator it;

#if 0
		cout << "Obj uncert: ";
		for(int i = 0; i < m_remaining; i++) {
			if( abs(testScores[m_dataIndex[i]]) < objMin ) {
				objMin = abs(testScores[m_dataIndex[i]]);
				objMinIdx = i;
			}
			cout << m_dataIndex[i] << ": " << testScores[m_dataIndex[i]] << ", ";
		}
		cout << endl << endl;

		cout << "Clust uncert: ";
		for(int i = 0; i < m_k; i++) {
			cout << i << ": " << clustScores[i] << ", ";
		}
		cout << endl << endl;

#if 0
		for(int c = 0; c < m_k; c++) {
			float 	*clustObjs = CreateCheckSet(c),
					*objScores = (float*)malloc(m_clusters[c].size() * sizeof(float));

			m_Classify->ScoreBatch(clustObjs, m_clusters[c].size(), m_dataset->GetDims(), objScores);

			if(  m_clusters[c].size() > 0 ) {
				cout << "Clust " << c << ": score: " << clustScores[c] << ", size: " << m_clusters[c].size()
						<< endl;
				cout << "Members: ";

				int i;
				for(it = m_clusters[c].begin(), i = 0; it != m_clusters[c].end(); it++, i++) {
					printf("%d: score: %2.6f ", *it, objScores[i]);
				}
			} else {
				cout << "Clust " << c << ": Empty";
			}
			cout << endl << endl;

			free(objScores);
			free(clustObjs);
		}
#endif
#endif

		// Find cluster closest to the hyperplane... Make sure the cluster's not empty
		float 	minScore = FLT_MAX, nextMinScore = FLT_MAX;
		int		minClustIdx = -1, nextMinClustIdx = -1;
		bool	empty = true;

		for(int i = 0; i < m_k; i++) {

			if( m_clusters[i].size() > 0 ) {
				if( abs(clustScores[i]) < minScore ) {

					empty = false;
					nextMinClustIdx = minClustIdx;
					nextMinScore = minScore;
					minClustIdx = i;
					minScore = abs(clustScores[i]);
				} else if( abs(clustScores[i]) < nextMinScore ) {
					nextMinClustIdx = i;
					nextMinScore = abs(clustScores[i]);
				}
			}
		}

		if( !empty ) {

			// Score the objects from the cluster
			float 	*clustObjs = CreateCheckSet(minClustIdx),
					*objScores = (float*)malloc(m_clusters[minClustIdx].size() * sizeof(float));
			int *clustObjLabels = (int*)malloc(m_clusters[minClustIdx].size() * sizeof(int));

			if( clustObjs && objScores && clustObjLabels ) {
				m_Classify->ScoreBatch(clustObjs, m_clusters[minClustIdx].size(), m_dataset->GetDims(), objScores);
				m_Classify->ClassifyBatch(clustObjs, m_clusters[minClustIdx].size(), m_dataset->GetDims(), clustObjLabels);
				free(clustObjs);
			}
#if 0
			cout << "Obj scores: ";
			for(int i = 0; i < m_clusters[minClustIdx].size(); i++)
				cout << objScores[i] << " ";
			cout << endl << endl;
#endif

			// Now find object closest to the hyperplane
			int		minNeg = -1, minPos = -1, nextMinNeg = -1, nextMinPos = -1,
					idx = 0;
			float 	minPosScore = FLT_MAX, minNegScore = FLT_MAX,
					nextMinPosScore = FLT_MAX, nextMinNegScore = FLT_MAX;

			for(it = m_clusters[minClustIdx].begin(); it != m_clusters[minClustIdx].end(); it++) {

				if( objScores[idx] > 0 ) {
					if( objScores[idx] < minPosScore ) {
						nextMinPosScore = minPosScore;
						nextMinPos = minPos;
						minPosScore = objScores[idx];
						minPos = *it;

					} else if( objScores[idx] < nextMinPosScore ) {
						nextMinPosScore = objScores[idx];
						nextMinPos = *it;
					}

				} else {
					if( abs(objScores[idx]) < minNegScore ) {
						nextMinNegScore = minNegScore;
						nextMinNeg = minNeg;
						minNegScore = abs(objScores[idx]);
						minNeg = *it;
					} else if( abs(objScores[idx]) < nextMinNegScore ) {
						nextMinNegScore = abs(objScores[idx]);
						nextMinNeg = *it;
					}
				}
				idx++;
			}

			if( minPosScore < abs(minNegScore) ) {
				obj = minPos;
				if( minNeg == -1 ) {
					other = nextMinPos;
				} else {
					other = minNeg;
				}
			} else {

				obj = minNeg;
				if( minPos == -1 ) {
					other = nextMinNeg;
				} else {
					other = minPos;
				}
			}

			// Remove the selected objects from the cluster centroid
			int clustSize = m_clusters[minClustIdx].size(),
				dims = m_dataset->GetDims();

			for(int i = 0; i < dims; i++) {
				m_centroids[minClustIdx][i] *= clustSize;
				m_centroids[minClustIdx][i] -= data[obj][i];
				if( other != -1 ) {
					m_centroids[minClustIdx][i] -= data[other][i];
					m_centroids[minClustIdx][i] /= (clustSize - 2);
				} else
					m_centroids[minClustIdx][i] /= (clustSize - 1);
			}
			m_clusters[minClustIdx].erase(obj);
			if( other != -1 )
				m_clusters[minClustIdx].erase(other);


#if 0
			cout << "Selected: " << obj << "(" << minClustIdx << "), searched: " << idx + m_k
				 << ", min: " << objMin << " (" << objMinIdx << ") clust: " << m_membership[objMinIdx] << endl;
#endif
			int swap = 0;
			while( m_dataIndex[swap] != obj )
				swap++;
			m_remaining--;
			m_dataIndex[swap] = m_dataIndex[m_remaining];

			if( other != -1 ) {
				swap = 0;
				while( m_dataIndex[swap] != other )
					swap++;
				m_remaining--;
				m_dataIndex[swap] = m_dataIndex[m_remaining];
			}

			if( objScores )
				free(objScores);
		}
	}
	return obj;
}


#endif



bool ClusterSample::ClusterData(void)
{
	bool	result = false;

	m_centroids = kmeans(m_dataset->GetData(), m_dataset->GetDims(), m_dataset->GetNumObjs(),
						 m_k, CHANGE_THRESH, m_membership);

	if( m_centroids ) {

		m_clusters = new set<int>[m_k];
		if( m_clusters ) {
			for(int i = 0; i < m_dataset->GetNumObjs(); i++) {
				m_clusters[m_membership[i]].insert(i);
			}
#if 0
			for(int i = 0; i < m_k; i++) {
				cout << i << ": ";
				set<int>::iterator it;
				for(it = m_clusters[i].begin(); it != m_clusters[i].end(); it++) {
					cout << *it << ", ";
				}
				cout << endl;
			}
#endif
			result = CalcMetrics();
		}
	}
	return result;
}






bool ClusterSample::CalcMetrics(void)
{
	bool	result = false;
	int		dims = m_dataset->GetDims();
	float	**data = m_dataset->GetData();

	m_compactness = (float*)malloc(m_k * sizeof(float));
	m_labelCnt = (LabelCnt*)malloc(m_k * sizeof(LabelCnt));

	if( m_compactness && m_labelCnt ) {

		// Clear label counts first
		memset(m_labelCnt, 0, m_k * sizeof(LabelCnt));

		set<int>::iterator	it;
		float	sum;

		for(int clust = 0; clust < m_k; clust++) {
			sum = 0.0f;
			for(it = m_clusters[clust].begin(); it != m_clusters[clust].end(); it++) {
				sum += euclid_dist_2(dims, m_centroids[clust], data[*it]);
			}
			m_compactness[clust] = sum / (float)m_clusters[clust].size();
		}
		result = true;
	}
	return result;
}






// Create a buffer of the items in the specified cluster.
//
float* ClusterSample::CreateCheckSet(int cluster, int altCluster)
{
	int		dims = m_dataset->GetDims(), size = m_clusters[cluster].size();
	float	*checkSet = NULL;

	if( altCluster != -1 )
		size += m_clusters[altCluster].size();

	checkSet = (float*)malloc(size * dims * sizeof(float));
	if( checkSet ) {
		float	**data = m_dataset->GetData();
		set<int>::iterator it;
		int		idx = 0;

		for(it = m_clusters[cluster].begin(); it != m_clusters[cluster].end(); it++)
			memcpy(&checkSet[idx++ * dims], data[*it], dims * sizeof(float));

		if( altCluster != -1 ) {
			for(it = m_clusters[altCluster].begin(); it != m_clusters[altCluster].end(); it++)
				memcpy(&checkSet[idx++ * dims], data[*it], dims * sizeof(float));
		}
	}
	return checkSet;
}



