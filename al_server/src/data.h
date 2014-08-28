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

		float	**GetData(void) { return m_objects; }
		int		*GetLabels(void) { return m_haveLabels ? m_labels : NULL;  }

		int		GetNumObjs(void) { return m_numObjs; }
		int		GetDims(void) { return m_numDim; }
		bool	HaveLabels(void) { return m_haveLabels; }

		int		FindItem(float xCent, float yCent, string slide);
		bool	GetSample(int index, float* sample);
		char	*GetSlide(int index);
		float	GetXCentroid(int index);
		float	GetYCentroid(int index);


protected:

		float	**m_objects;
		int		*m_labels;
		float	*m_xCentroid;
		float	*m_yCentroid;
		int		*m_slideIdx;
		char	**m_slides;

		bool	m_haveLabels;
		int		m_numObjs;
		int		m_numDim;
		int		m_numSlides;

		hid_t	m_space;			// For cleaning up slide names
		hid_t	m_memType;

		void 	Cleanup(void);
};


#endif
