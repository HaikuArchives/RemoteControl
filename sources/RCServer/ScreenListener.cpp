#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>

#include "autodelete.h"
#include "ClientIO.h"
#include "ScreenServer.h"
#include "ScreenListener.h"

ScreenListener::ScreenListener(int32 sock, char const *passwd) :
	mSock(sock),
	mPasswd(passwd)
{
	mThid=spawn_thread(threadFunc, "screen listener", B_NORMAL_PRIORITY, this);
	if(mThid<0)
	{
		cerr <<"couldn't spawn thread -- " <<strerror(mThid) <<endl;
		closesocket(mSock);
		delete this;
		return;
	}
	if(resume_thread(mThid)!=B_OK)
	{
		closesocket(mSock);
		kill_thread(mThid);
		delete this;
		return;
	}
}

ScreenListener::~ScreenListener()
{
}

int32 ScreenListener::threadFunc(void *cookie)
{
	return ((ScreenListener *)cookie)->ThreadFunc();
}

int32 ScreenListener::ThreadFunc()
{
	char timestr[256];
	time_t timet=time(0);
	strftime(timestr, 256, "%c", localtime(&timet));
	cout <<timestr <<"\tserver ready" <<endl;
	
	autodelete<ScreenListener> deleteThis(this);
	
	int32 client;
	struct sockaddr sa;
	struct sockaddr_in *sin;
	int size;
	while((client=accept(mSock, &sa, &size))>=0)
	{
		sin=(sockaddr_in *)&sa;
		new ScreenServer(new ClientIO(client), sin->sin_addr.s_addr, mPasswd);
	}
	cerr <<"couldn't accept -- " <<strerror(errno) <<endl;
	return B_OK;
}
