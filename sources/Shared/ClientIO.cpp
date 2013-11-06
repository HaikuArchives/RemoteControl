#include <iostream>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

#include <Debug.h>

#include "ClientIO.h"

ClientIO::ClientIO(int32 sock) :
	mSock(sock)
{
	mInPos=mInSize=0;
	mOutPos=0;
}

ClientIO::~ClientIO()
{
	closesocket(mSock);
}

void ClientIO::Flush()
{
	if(!mOutPos)
		return;
	
	int32 pos=0;
	do
	{
		int32 s;
		do {s=send(mSock, &(mOutBuffer[pos]), mOutPos, 0);} while(s<0 && errno==EINTR);
		
		if(s<=0)
			throw ClientDisconnected();
		mOutPos-=s;
		pos+=s;
	} while(mOutPos);
}

void ClientIO::Fill()
{
	int32 size;
	
	do {size=recv(mSock, mInBuffer, BUFFSIZE, 0);} while(size<0 && errno==EINTR);
	
	if(size<=0)
		throw ClientDisconnected();
	mInPos=0;
	mInSize=size;
}

void ClientIO::ReadBytes(void *buf, int32 size)
{
	if(size<=mInSize-mInPos)
	{
		memcpy(buf, &(mInBuffer[mInPos]), size);
		mInPos+=size;
		return;
	}
	
	while(size)
	{
		int32 toread=min_c(size, mInSize-mInPos);
		memcpy(buf, &(mInBuffer[mInPos]), toread);
		mInPos+=toread;
		size-=toread;
		(int32 &)buf+=toread;
		if(size)
			Fill();
	}
}

void ClientIO::WriteBytes(const void *buf, int32 size)
{
	do
	{
		if(BUFFSIZE==mOutPos)
			Flush();
		int32 tosend=min_c(BUFFSIZE-mOutPos, size);
		memcpy(&(mOutBuffer[mOutPos]), buf, tosend);
		mOutPos+=tosend;
		buf=((char *)buf)+tosend;
		size-=tosend;
	} while(size);
}

int32 ClientIO::CloseSocket()
{
	return closesocket(mSock);
}
