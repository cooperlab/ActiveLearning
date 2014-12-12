#include <fstream>
#include <ctime>
#include <sys/time.h>
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
				ltm->tm_mon + 1, ltm->tm_mday, 1900 + ltm->tm_year,
				ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
		m_logFile << typeName << " " << timeBuff << " - " << msg << endl;
		result = true;
	}
	return result;
}




double EvtLogger::WallTime(void)
{
    double          now_time;
    struct timeval  etstart;
    struct timezone tzp;

    if (gettimeofday(&etstart, &tzp) == -1)
        perror("Error: calling gettimeofday() not successful.\n");

    now_time = ((double)etstart.tv_sec) +              /* in seconds */
               ((double)etstart.tv_usec) / 1000000.0;  /* in microseconds */
    return now_time;
}
