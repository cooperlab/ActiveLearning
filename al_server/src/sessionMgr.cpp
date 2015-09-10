//
//	Copyright (c) 2014-2015, Emory University
//	All rights reserved.
//
//	Redistribution and use in source and binary forms, with or without modification, are
//	permitted provided that the following conditions are met:
//
//	1. Redistributions of source code must retain the above copyright notice, this list of
//	conditions and the following disclaimer.
//
//	2. Redistributions in binary form must reproduce the above copyright notice, this list
// 	of conditions and the following disclaimer in the documentation and/or other materials
//	provided with the distribution.
//
//	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//	SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
//	WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//	DAMAGE.
//
//
#include <string>
#include <thread>
#include <ctime>
#include <unistd.h>
#include <jansson.h>

#include "sessionMgr.h"
#include "logger.h"
#include "commands.h"


extern EvtLogger	*gLogger;



SessionMgr::SessionMgr(string dataPath, string outPath, string heatmapPath,
						int sessionTimeout) :
m_dataPath(dataPath),
m_outPath(outPath),
m_heatmapPath(heatmapPath),
m_sessionTimeout(sessionTimeout)
{

}



SessionMgr::~SessionMgr(void)
{

}




bool SessionMgr::HandleRequest(const int sock, string data)
{
	// Just create a thread to handle the command and immeadiatly detach
	//
	std::thread(&SessionMgr::ParseCommand, this, sock, data).detach();

	return true;
}




void SessionMgr::ParseCommand(const int sock, string data)
{
	bool	result = true;
	json_t		*root;
	json_error_t error;

	root = json_loads(data.c_str(), 0, &error);
	if( !root ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "[HandleRequest] Error parsing json");
		result = false;
	}

	if( result ) {
		json_t	*uidObj = json_object_get(root, UID_TAG);
		const char *uid = json_string_value(uidObj);

		if( uid != NULL ) {
			json_t	*cmdObj = json_object_get(root, CMD_TAG);

			if( !json_is_string(cmdObj) ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Command is not a string");
				result = false;
			} else {
				const char	*command = json_string_value(cmdObj);
				if( strncmp(command, CMD_INIT, strlen(CMD_INIT)) == 0 ||
					strncmp(command, CMD_PICKINIT, strlen(CMD_PICKINIT)) == 0 ||
					strncmp(command, CMD_CLASSINIT, strlen(CMD_CLASSINIT)) == 0 ) {

					result = CreateSession(uid, data, sock);

				} else if( strncmp(command, CMD_END, strlen(CMD_END)) == 0 ||
						   strncmp(command, CMD_FINAL, strlen(CMD_FINAL)) == 0 ||
						   strncmp(command, CMD_PICKEND, strlen(CMD_PICKEND)) == 0 ||
						   strncmp(command, CMD_CLASSEND, strlen(CMD_CLASSEND))== 0 ) {

					result = EndSession(uid, data, sock);

				} else {
					result = CmdSession(uid, data, sock);
				}
			}
		} else {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "[HandleRequest] Not a session request");
		}
	}

	// Check for session timeout
	time_t now;
	double elapsedTime;

	time(&now);
	for(int i = 0; i < m_sessions.size(); i++) {
		elapsedTime =  difftime(now, m_sessions[i]->touchTime);
		if( elapsedTime > (double)m_sessionTimeout ) {

			delete m_sessions[i]->learner;
			delete m_sessions[i];
			m_sessions.erase(m_sessions.begin() + i);
		}
	}
	::close(sock);
}





bool SessionMgr::CreateSession(string uid, string data, int sock)
{
	bool	result = true;
	Session *session = new Session;

	if( session == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create session object");
		result = false;
	} else {
		session->UID = uid;
		session->learner = new Learner(m_dataPath, m_outPath, m_heatmapPath+uid);
		time(&session->touchTime);
		m_sessions.push_back(session);

		// Create new directory for heatmaps
		//
		string cmd = "mkdir " + m_heatmapPath + uid;
		int ret = system(cmd.c_str());

		result = session->learner->ParseCommand(sock, data.c_str(), data.size());
	}
	return result;
}





bool SessionMgr::EndSession(string uid, string data, int sock)
{
	bool	result = true;

	for(int i = 0; i < m_sessions.size(); i++) {
		if( m_sessions[i]->UID.compare(uid) == 0 ) {
			result = m_sessions[i]->learner->ParseCommand(sock, data.c_str(), data.size());

			delete m_sessions[i]->learner;
			delete m_sessions[i];
			m_sessions.erase(m_sessions.begin() + i);

			// Remove directory for heatmaps
			string cmd = "rm " + m_heatmapPath + uid + "/*.jpg";
			int ret = system(cmd.c_str());

			cmd = "rmdir " + m_heatmapPath + uid;
			ret = system(cmd.c_str());
		}
	}
	return result;
}





bool SessionMgr::CmdSession(string uid, string data, int sock)
{
	bool	result = true;

	for(int i = 0; i < m_sessions.size(); i++) {
			if( m_sessions[i]->UID.compare(uid) == 0 ) {

				time(&m_sessions[i]->touchTime);
				result = m_sessions[i]->learner->ParseCommand(sock, data.c_str(), data.size());
		}
	}
	return result;
}
