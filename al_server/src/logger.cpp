#include <fstream>
#include <iostream>
#include <ctime>
#include <cstdarg>
#include <cstdio>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "logger.h"


using namespace std;


#define LOGFILE_PATH_LENGTH		1024





EvtLogger::EvtLogger(string logFile)
{
	// Check for existing file first
	struct stat buffer;

	if( stat(logFile.c_str(), &buffer) == 0 ) {
		// Check last modification day, rename if not today
		//
		time_t	now = time(0);
		tm	*ltime = localtime(&buffer.st_mtime);
		int  todayDay, modDay = ltime->tm_wday;

		ltime = localtime(&now);
		todayDay = ltime->tm_wday;

		if( modDay != todayDay ) {
			char newName[LOGFILE_PATH_LENGTH];
			// Rename the file with the days since sunday appended
			// to the end.
			//
			snprintf(newName, LOGFILE_PATH_LENGTH, "%s.%d", logFile.c_str(), modDay);
			rename(logFile.c_str(), newName);
		}
	}
	m_logFile.open(logFile.c_str(), std::ofstream::out | std::ofstream::app);

	// Start log monitor
	pthread_create(&m_threadId, NULL, EvtLogger::ThreadEntry, (void*)this);
}






EvtLogger::~EvtLogger(void)
{
	if( m_logFile.is_open() )
		m_logFile.close();
}







bool EvtLogger::LogMsgv(LogType type, const char* msg, ...)
{
	bool	result = true;
	char	buffer[1024];

	va_list	args;

	va_start(args, msg);
	vsnprintf(buffer, 1024, msg, args);
	va_end(args);

	result = LogMsg(type, buffer);

	return result;
}







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






void *EvtLogger::ThreadEntry(void *self)
{
	EvtLogger *logger = (EvtLogger*)self;
	logger->Monitor();

	return NULL;
}






void EvtLogger::Monitor(void)
{
	unsigned	secondsTilMidnight;
	time_t		now;
	tm			*ltime;

	while(1) {
		// Sleep until midnight
		now = time(0);
		ltime = localtime(&now);

		secondsTilMidnight = (23 - ltime->tm_hour) * 3600;
		secondsTilMidnight += ((59 - ltime->tm_min) * 60);
		secondsTilMidnight += (59 - ltime->tm_sec);

		sleep(secondsTilMidnight);

		// Archive current log file
		m_logFile.close();
		char	newName[LOGFILE_PATH_LENGTH];

		snprintf(newName, LOGFILE_PATH_LENGTH, "%s.%d", m_fqfn.c_str(), ltime->tm_wday);
		rename(m_fqfn.c_str(), newName);

		m_logFile.open(m_fqfn.c_str(), std::ofstream::out | std::ofstream::trunc);
	}
}

