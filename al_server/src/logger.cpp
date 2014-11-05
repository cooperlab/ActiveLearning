#include <fstream>
#include <ctime>
#include "logger.h"


using namespace std;



EvtLogger::EvtLogger(string logFile)
{
	m_logFile.open(logFile.c_str(), std::ofstream::out | std::ofstream::app);
}




EvtLogger::~EvtLogger(void)
{
	if( m_logFile.is_open() )
		m_logFile.close();
}



// TODO - Add a formatting capable log method



bool EvtLogger::LogMsg(LogType type, string msg)
{
	bool	result = false;
	time_t	now = time(0);

	if( m_logFile.is_open() ) {
		tm	*ltm = localtime(&now);
		char	timeBuff[100];
		string	typeName;

		switch( type ) {
		case Evt_ERROR:
			typeName = "[ERROR]";
			break;
		case Evt_WARN:
			typeName = "[WARN ]";
			break;
		case Evt_INFO:
			typeName = "[INFO ]";
			break;
		default:
			typeName = "[?????]";
			break;
		}
		snprintf(timeBuff, 100, "%02d-%02d-%d %02d:%02d:%02d",
				ltm->tm_mon, ltm->tm_mday, 1900 + ltm->tm_year,
				1 + ltm->tm_hour, 1 + ltm->tm_min, 1 + ltm->tm_sec);
		m_logFile << typeName << " " << timeBuff << " - " << msg << endl;
		result = true;
	}
	return result;
}
