#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>

#include "autodelete.h"
#include "ClientIO.h"
#include "InputServer.h"
#include "InputListener.h"

using namespace std;

InputListener::InputListener(int32 sock, char const *passwd) :
	mSock(sock),
	mPasswd(passwd)
{
	mThid=spawn_thread(threadFunc, "input listener", B_NORMAL_PRIORITY, this);
	if(mThid<0)
	{
		cerr <<"couldn't spawn thread -- " <<strerror(mThid) <<endl;
		close(mSock);
		delete this;
		return;
	}
	if(resume_thread(mThid)!=B_OK)
	{
		close(mSock);
		kill_thread(mThid);
		delete this;
		return;
	}
}

InputListener::~InputListener()
{
}

int32 InputListener::threadFunc(void *cookie)
{
	return ((InputListener *)cookie)->ThreadFunc();
}

int32 InputListener::ThreadFunc()
{
	autodelete<InputListener> deleteThis(this);
	
	int32 client;
	struct sockaddr sa;
	socklen_t size;
	while((client=accept(mSock, &sa, &size))>=0)
		new InputServer(new ClientIO(client), mPasswd);
	cerr <<"couldn't accept -- " <<strerror(errno) <<endl;
	return B_OK;
}
