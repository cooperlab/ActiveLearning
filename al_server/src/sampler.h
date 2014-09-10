#if !defined(_SAMPLER_H_)
#define _SAMPLER_H_

#include "classifier.h"
#include "data.h"





class Sampler
{
public:
				Sampler(MData *dataset);
	virtual		~Sampler(void);

	enum SampType {SAMP_RANDOM, SAMP_UNCERTAIN, SAMP_INFO_DENSITY, SAMP_HIERARCHICAL, SAMP_CLUSTER};

	virtual int Select(float *score = NULL) = 0;
	virtual bool Init(int count, int *list);
	virtual SampType GetSamplerType(void) = 0;

protected:

	MData	*m_dataset;
	int		*m_dataIndex;
	int		m_remaining;

};





//-----------------------------------------------------------------------------




class UncertainSample : public Sampler
{
public:
				UncertainSample(Classifier *classify, MData *dataset);
	virtual		~UncertainSample(void);

	virtual int		Select(float *score = NULL);
	virtual SampType 	GetSamplerType(void) { return SAMP_UNCERTAIN; }

protected:

	Classifier *m_Classify;
	float*	CreateCheckSet(void);
};




#endif
