#include <iostream>
#include <string.h>
#include <sys/socket.h>

#include "autodelete.h"
#include "ClientIO.h"
#include "InputServer.h"
#include "proto.h"
#include "Password.h"

InputServer::InputServer(ClientIO *clio, char const *passwd) :
	mClio(clio),
	mThid(B_ERROR),
	mPort(B_ERROR),
	mFilterPort(B_ERROR),
	mPasswd(passwd)
{
	mPort=find_port(PortName);
	if(mPort<0)
	{
		cerr <<"couldn't find port -- " <<strerror(mPort) <<endl;
		delete this;
		return;
	}
	
	mFilterPort=find_port(FilterPortName);
	if(mFilterPort<0)
	{
		cerr <<"couldn't find filter port -- " <<strerror(mFilterPort) <<endl;
		delete this;
		return;
	}
	
	mThid=spawn_thread(threadFunc, "input server", B_NORMAL_PRIORITY, this);
	if(mThid<0)
	{
		cerr <<"couldn't spawn thread -- " <<strerror(mThid) <<endl;
		delete this;
		return;
	}
	status_t err;
	if((err=resume_thread(mThid))!=B_OK)
	{
		cerr <<"couldn't resume thread -- " <<strerror(err) <<endl;
		kill_thread(mThid);
		delete this;
		return;
	}
}

InputServer::~InputServer()
{
	mClio->CloseSocket();
	if(mThid>0)
	{
		status_t ret;
		wait_for_thread(mThid, &ret);
	}
	delete mClio;
}

int32 InputServer::threadFunc(void *cookie)
{
	InputServer *ptr=((InputServer *)cookie);
	status_t err=B_OK;
	
	try
	{
		err=ptr->ThreadFunc();
	}
	catch(ClientDisconnected)
	{
	}
	
	ptr->mThid=B_ERROR;
	delete ptr;
	
	return err;
}

int32 InputServer::ThreadFunc()
{
	if(!Password(mClio).RecvPassword(mPasswd))
		return B_ERROR;
	
	while(1)
	{
		int32 code;
		
		mClio->ReadBytes(&code, sizeof(int32));
		switch(code)
		{
			case RCMessage::FlatMessage:
			{
				int32 size;
				mClio->ReadBytes(&size, sizeof(int32));
				
				char *flat=new char[size];
				autodelete<char> deleteFlat(flat);
				
				mClio->ReadBytes(flat, size);
				if(write_port_etc(mPort, MsgCode, flat, size, B_TIMEOUT, 2000000)!=B_OK)
					return B_ERROR;
				break;
			}
			case RCMessage::EnableFilter:
			case RCMessage::DisableFilter:
				if(write_port_etc(mFilterPort, code, 0, 0, B_TIMEOUT, 2000000)!=B_OK)
					return B_ERROR;
				break;
		}
	}
	return B_OK;
}
