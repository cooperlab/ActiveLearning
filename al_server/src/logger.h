#if !defined(_LOGGER_H_)
#define _LOGGER_H_

#include <fstream>




class EvtLogger
{
public:
	enum LogType {Evt_ERROR, Evt_INFO, Evt_WARN};

			EvtLogger(std::string logFile);
			~EvtLogger();

	bool	LogMsg(LogType tyep, std::string msg);

	double 	WallTime(void);



protected:

	std::ofstream	m_logFile;

};


#endif
