#if !defined(_LEARNER_H_)
#define _LEARNER_H_

#include <vector>
#include <jansson.h>

#include "data.h"
#include "ocvsvm.h"
#include "sampler.h"


#define UID_LENGTH 		23




class Learner
{
public:
			Learner(string dataPath = "./");
			~Learner(void);

	bool	ParseCommand(const int sock, char *data, int size);

	// TODO - Add method for in-sample error

protected:

	char	m_UID[UID_LENGTH + 1];
	MData	*m_dataset;
	string	m_dataPath;

	vector<int> m_samples;
	int			*m_labels;
	int			*m_ids;
	float		*m_trainSet;
	int			m_iteration;

	Classifier 	*m_classifier;
	Sampler		*m_sampler;

	bool	StartSession(const int sock, json_t *obj);
	bool	Select(const int sock, json_t *obj);
	bool	Submit(const int sock, json_t *obj);
	bool	Prime(const int sock, json_t *obj);
	bool	CancelSession(const int sock, json_t *obj);
	bool	FinalizeSession(const int sock, json_t *obj);

	bool 	UpdateBuffers(int updateSize);
	void	Cleanup(void);
};




#endif
