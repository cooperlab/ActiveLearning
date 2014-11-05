/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*   File:         seq_kmeans.c  (sequential version)                        */
/*   Description:  Implementation of simple k-means clustering algorithm     */
/*                 This program takes an array of N data objects, each with  */
/*                 M coordinates and performs a k-means clustering given a   */
/*                 user-provided value of the number of clusters (K). The    */
/*                 clustering results are saved in 2 arrays:                 */
/*                 1. a returned array of size [K][N] indicating the center  */
/*                    coordinates of K clusters                              */
/*                 2. membership[N] stores the cluster center ids, each      */
/*                    corresponding to the cluster a data object is assigned */
/*                                                                           */
/*   Author:  Wei-keng Liao                                                  */
/*            ECE Department, Northwestern University                        */
/*            email: wkliao@ece.northwestern.edu                             */
/*   Copyright, 2005, Wei-keng Liao                                          */
/*   See COPYRIGHT notice in top-level directory.                            */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <cstdlib>



using namespace std;




/*----< euclid_dist_2() >----------------------------------------------------*/
/* square of Euclid distance between two multi-dimensional points            */
__inline float euclid_dist_2(int    numdims,  /* no. dimensions */
              				float *coord1,   /* [numdims] */
               				float *coord2)   /* [numdims] */
{
    int i;
    float ans = 0.0;

    for (i = 0; i < numdims; i++)
        ans += (coord1[i] - coord2[i]) * (coord1[i] - coord2[i]);

    return ans;
}







/*----< find_nearest_cluster() >---------------------------------------------*/
__inline static int find_nearest_cluster(int	numClusters, /* no. clusters */
                         				 int	numCoords,   /* no. coordinates */
                         				 float	*object,      /* [numCoords] */
                         				 float	**clusters)    /* [numClusters][numCoords] */
{
    int		index;
    float 	dist, min_dist;

    /* find the cluster id that has min distance to object */
    index    = 0;
    min_dist = euclid_dist_2(numCoords, object, clusters[0]);

	for( int i = 1; i < numClusters; i++) {
		dist = euclid_dist_2(numCoords, object, clusters[i]);
		/* no need for square root */
		if( dist < min_dist ) { /* find the min and its array index */
			min_dist = dist;
			index = i;
		}
	}
    return	index;
}



#define DETERMINISTIC  0


/*----< seq_kmeans() >-------------------------------------------------------*/
/* return an array of cluster centers of size [numClusters][numCoords]       */
float** kmeans(float **objects,      /* in: [numObjs][numCoords] */
                   int     numCoords,    /* no. features */
                   int     numObjs,      /* no. objects */
                   int     numClusters,  /* no. clusters */
                   float   threshold,    /* % objects change membership */
                   int    *membership)   /* out: [numObjs] */
{
    int     index, loop=0;
    int     *newClusterSize; /* [numClusters]: no. objects assigned in each
                                new cluster */
    float	delta;          /* % of objects change their clusters */
	float	**clusters;       /* out: [numClusters][numCoords] */
	float	**newClusters;    /* [numClusters][numCoords] */
	bool	result = true;

	/* allocate a 2D space for returning variable clusters[] (coordinates
		of cluster centers) */
	clusters = (float**) malloc(numClusters * sizeof(float*));
	if( clusters ) {
		clusters[0] = (float*)  malloc(numClusters * numCoords * sizeof(float));
		for(int i = 1; i < numClusters; i++)
			clusters[i] = clusters[i - 1] + numCoords;
	} else {
		result = false;
	}
	
#if DETERMINISTIC

	if( result ) {
		// Just select the first k objects
		for(int i = 0; i < numClusters; i++) {

			for(int dim = 0; dim < numCoords; dim++)
				clusters[i][dim] = objects[i][dim];
		}
	}
#else
	if( result ) {

		int	 *pickList = (int*)malloc(numObjs * sizeof(int));
		if( pickList ) {
			for(int i = 0; i < numObjs; i++)
				pickList[i] = i;
			
			int pick, remaining = numObjs;
		
			for(int i = 0; i < numClusters; i++) {
				pick = rand() % remaining;
			
				for(int dim = 0; dim < numCoords; dim++)
					clusters[i][dim] = objects[pickList[pick]][dim];
				
				remaining--;
				pickList[pick] = pickList[remaining];
			}
		} else {
			result = false;
		}
	}
#endif
	
	if( result ) {
		/* initialize membership[] */
		for(int i = 0; i < numObjs; i++) 
			membership[i] = -1;
	}
	
	
	if( result ) {
		/* need to initialize newClusterSize and newClusters[0] to all 0 */
		newClusterSize = (int*)calloc(numClusters, sizeof(int));
		newClusters = (float**)malloc(numClusters * sizeof(float*));
		if( newClusters ) {
			newClusters[0] = (float*)calloc(numClusters * numCoords, sizeof(float));
			if( newClusters[0] ) {
				for(int i = 1; i<numClusters; i++)
					newClusters[i] = newClusters[i - 1] + numCoords;
			} else {
				result = false;
			}
		} else {
			result = false;
		}
	}
	
	
	if( result ) {
	
		do {
			delta = 0.0;
			
			// Assign each object to the nearest cluster
			for(int i = 0; i < numObjs; i++) {

				index = find_nearest_cluster(numClusters, numCoords, objects[i], clusters);

				/* if membership changes, increase delta by 1 */
				if( membership[i] != index) 
					delta += 1.0;

				/* assign the membership to object i */
				membership[i] = index;

				/* update new cluster centers : sum of objects located within */
				newClusterSize[index]++;
				for(int j = 0; j < numCoords; j++)
					newClusters[index][j] += objects[i][j];
			}

			// Calculate new cluster centroids
			for(int i = 0; i < numClusters; i++) {
				for(int j = 0; j < numCoords; j++) {
					if( newClusterSize[i] > 0 )
						clusters[i][j] = newClusters[i][j] / newClusterSize[i];
					newClusters[i][j] = 0.0;   /* set back to 0 */
				}
				newClusterSize[i] = 0;   /* set back to 0 */
			}

			delta /= numObjs;
		} while (delta > threshold && loop++ < 500);
	}
	if( newClusters ) {
		if( newClusters[0] )
			free(newClusters[0]);
		free(newClusters);
	}
	if( newClusterSize )
		free(newClusterSize);
		
	return clusters;
}

