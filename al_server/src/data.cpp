#include <iostream>

#include "data.h"


using namespace std;



MData::MData(void) :
m_numObjs(0),
m_haveLabels(false),
m_objects(NULL),
m_labels(NULL),
m_xCentroid(NULL),
m_yCentroid(NULL),
m_slideIdx(NULL),
m_slides(NULL)
{

}






MData::~MData(void)
{
	Cleanup();
}





void MData::Cleanup(void)
{
	if( m_objects ) {
		if( m_objects[0] )
			free(m_objects[0]);
		free(m_objects);
		m_objects = NULL;
	}

	if( m_labels ) {
		free(m_labels);
		m_labels = NULL;
	}
	if( m_xCentroid ) {
		free(m_xCentroid);
		m_xCentroid = NULL;
	}
	if( m_yCentroid ) {
		free(m_yCentroid);
		m_yCentroid = NULL;
	}
	if( m_slideIdx ) {
		free(m_slideIdx);
		m_slideIdx = NULL;
	}

	if( m_slides ) {
		// Release mem allocated by H5Dread
		H5Dvlen_reclaim(m_memType, m_space, H5P_DEFAULT, m_slides);
		free(m_slides);
		m_slides = NULL;
	}
}






//
//	Loads the data from an HDF5 file. There should be 2 datasets:
//	/features and /labels. The features data should be 2D in row-major
//	order, with each row representing an object.
//
//
bool MData::Load(string fileName)
{
	bool		result = true;
	hid_t		fileId;
	hsize_t		dims[2];
	herr_t		status;

	fileId = H5Fopen(fileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
	if( fileId < 0 ) {
		cerr << "Unable to open " << fileName << endl;
		result = false;
	}

	if( result ) {
		status = H5LTget_dataset_info(fileId, "/features", dims, NULL, NULL);
		if( status < 0 ) {
			cerr << "Unable to read features dimensions" << endl;
			result = false;
		}
	}

	// Allocate buffer for the labels (if there are any)
	if( result && H5Lexists(fileId, "/labels", H5P_DEFAULT) ) {
		m_labels = (int*)malloc(dims[0] * sizeof(int));
		if( m_labels == NULL ) {
			cerr << "Unable to allocate label buffer" << endl;
			result = false;
		}

		// Read label data
		if( result ) {
			status = H5LTread_dataset_int(fileId, "/labels", m_labels);
			if( status < 0 ) {
				cerr << "Unable to read label data" << endl;
				result = false;
			} else {
				m_haveLabels = true;
			}
		}
	}

	// Allocate buffer for slide indices
	//
	if( result ) {
		m_slideIdx = (int*)malloc(dims[0] * sizeof(int));
		if( m_slideIdx == NULL ) {
			cerr << "Unable to allocate index buffer" << endl;
			result = false;
		}
	}

	// Read slide indices
	if( result ) {
		status = H5LTread_dataset_int(fileId, "/slideIdx", m_slideIdx);
		if( status < 0 ) {
			cerr << "Unable to read index data" << endl;
			result = false;
		}
	}

	// Allocate buffer for x_centroid
	//
	if( result ) {
		m_xCentroid = (float*)malloc(dims[0] * sizeof(float));
		if( m_xCentroid == NULL ) {
			cerr << "Unable to allocate x centroids buffer" << endl;
			result = false;
		}
	}
	// Read x centroids
	if( result ) {
		status = H5LTread_dataset_float(fileId, "/x_centroid", m_xCentroid);
		if( status < 0 ) {
			cerr << "Unable to read x centroid data" << endl;
			result = false;
		}
	}

	// Allocate buffer for y centroids
	//
	if( result ) {
		m_yCentroid = (float*)malloc(dims[0] * sizeof(float));
		if( m_yCentroid == NULL ) {
			cerr << "Unable to allocate y centroids buffer" << endl;
			result = false;
		}
	}

	// Read y centroids
	if( result ) {
		status = H5LTread_dataset_float(fileId, "/y_centroid", m_yCentroid);
		if( status < 0 ) {
			cerr << "Unable to read y centroid data" << endl;
			result = false;
		}
	}

	// Allocate a buffer for the feature data
	//
	if( result ) {
		m_objects = (float**)malloc(dims[0] * sizeof(float*));
		if( m_objects ) {
			m_objects[0] = (float*)malloc(dims[0] * dims[1] * sizeof(float));
			if( m_objects[0] ) {
				for(int i = 1; i < dims[0]; i++)
					m_objects[i] = m_objects[i - 1] + dims[1];
			} else {
				cerr << "Unable to allocate feature buffer" << endl;
				result = false;
			}
		} else {
			cerr << "Unable to allocate feature buffer index" << endl;
			result = false;
		}
	}

	// Read feature data
	//
	if( result ) {
		status = H5LTread_dataset_float(fileId, "/features", m_objects[0]);
		if( status < 0 ) {
			cerr << "Unable to read feature data" << endl;
			result = false;
		} else {
			m_numObjs = dims[0];
			m_numDim = dims[1];
		}
	}

	// Read slide names, do this last because we reuse the dims variable
	//
	hid_t	dset, fileType;
	if( result ) {
		dset = H5Dopen(fileId, "slides", H5P_DEFAULT);
		fileType = H5Dget_type(dset);
		m_space = H5Dget_space(dset);
		H5Sget_simple_extent_dims(m_space, dims, NULL);

		m_slides = (char**)malloc(dims[0] * sizeof(char*));
		if( m_slides == NULL ) {
			cerr << "Unable to allocate slide name buffer" << endl;
			result = false;
		}
	}

	if( result ) {
		m_memType = H5Tcopy(H5T_C_S1);
		H5Tset_size(m_memType, H5T_VARIABLE);
		status = H5Dread(dset, m_memType, H5S_ALL, H5S_ALL, H5P_DEFAULT, m_slides);
		if( status < 0 ) {
			cerr << "Unable to read slide names" << endl;
			result = false;
		} else {
			this->m_numSlides = dims[0];
			H5Dclose(dset);
			H5Tclose(fileType);
		}
	}
	if( fileId > 0 )
		H5Fclose(fileId);

	return result;
}







int MData::FindItem(float xCent, float yCent, string slide)
{
	int		idx = -1, slideIdx = -1;

	for(int i = 0; i < m_numSlides; i++) {
		if( slide.compare(m_slides[i]) == 0 ) {
			slideIdx = i;
			break;
		}
	}

	if( slideIdx != -1 ) {
		for(int i = 0; i < m_numObjs; i++) {
			if( xCent == m_xCentroid[i] &&
				yCent == m_yCentroid[i] &&
				slideIdx == m_slideIdx[i] ) {

				idx = i;
				break;
			}
		}
	}
	return idx;
}





bool MData::GetSample(int index, float *sample)
{
	bool	result = false;

	if( index < m_numObjs ) {
		memcpy(sample, m_objects[index], m_numDim * sizeof(float));
		result = true;
	}
	return result;
}





char *MData::GetSlide(int index)
{
	char	*slide = NULL;

	if( index < m_numObjs ) {
		int	slideIdx = m_slideIdx[index];
		slide = m_slides[slideIdx];
	}
	return slide;
}




float MData::GetXCentroid(int index)
{
	float	xCent = -1.0;

	if( index < m_numObjs ) {
		xCent = m_xCentroid[index];
	}
	return xCent;
}



float MData::GetYCentroid(int index)
{
	float	yCent = -1.0;

	if( index < m_numObjs ) {
		yCent = m_yCentroid[index];
	}
	return yCent;
}
