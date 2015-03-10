#if !defined(_DATA_H_)
#define _DATA_H_

#include <string>

#include "hdf5.h"
#include "hdf5_hl.h"




using namespace std;


class MData
{
public:
		MData(void);
		~MData(void);


		bool	Load(string fileName);
		bool	Create(float *dataSet, int numObjs, int numDims, int *labels,
						int *ids, int *iteration, float *means, float *stdDev,
						float *xCentroid, float *yCentroid, char **slides,
						int *slideIdx, int slideCnt);
		bool	SaveAs(string filename);

		float	**GetData(void) { return m_objects; }
		int		*GetLabels(void) { return m_haveLabels ? m_labels : NULL;  }
		char	**GetSlideNames(void)  { return m_slides; }
		int		*GetSlideIndices(void) { return m_slideIdx; }

		int		GetNumObjs(void) { return m_numObjs; }
		int		GetDims(void) { return m_numDim; }
		int		GetNumSlides(void) { return m_numSlides; }
		bool	HaveLabels(void) { return m_haveLabels; }

		int		FindItem(float xCent, float yCent, string slide);
		bool	GetSample(int index, float* sample);
		char	*GetSlide(int index);
		int		GetSlideIdx(const char *slide);
		float	GetXCentroid(int index);
		float	GetYCentroid(int index);
		int		GetIteration(int index) { return m_iteration[index]; }
		float	*GetMeans(void) { return m_means; }
		float	*GetStdDevs(void)  { return m_stdDevs; }

		float 	*GetSlideData(const string slide, int& numSlideObjs);
		int		GetSlideOffset(const string slide, int& numSlideObjs);

protected:

		float	**m_objects;
		int		*m_labels;
		float	*m_xCentroid;
		float	*m_yCentroid;
		int		*m_slideIdx;
		char	**m_slides;

		bool	m_haveIters;
		int		*m_iteration;

		bool	m_haveLabels;
		int		m_numObjs;
		int		m_numDim;
		int		m_numSlides;

		bool	m_haveDbIds;
		int		*m_dbIds;

		bool 	m_created;

		int		*m_dataIdx;		// Index where the corresponding slide's feature
								// data starts.
		// Normalization parameters
		//
		float	*m_means;
		float	*m_stdDevs;

		hid_t	m_space;			// For cleaning up slide names
		hid_t	m_memType;

		void 	Cleanup(void);
		bool	SaveProvenance(hid_t fileId);
		bool	CreateSlideData(char **slides, int *slideIdx, int numSlides, int numObjs);

};


#endif
