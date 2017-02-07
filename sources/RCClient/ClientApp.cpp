#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>

#include "ControlWindow.h"
#include "InputClient.h"
#include "ScreenClient.h"
#include "ClientWin.h"
#include "ClientIO.h"

#include "ClientApp.h"

using namespace std;

ClientApp::ClientApp() :
	BApplication("application/x-vnd.Titoz-RemoteControlClient"),
	mInputClient(0),
	mScreenClient(0),
	mArgsReceived(false),
	mPorti(4321),
	mPorts(4322),
	mPasswd(0)
{
}

ClientApp::~ClientApp()
{
	if(mInputClient)
		delete mInputClient;
	if(mScreenClient)
		delete mScreenClient;
}

void ClientApp::usage(char *name)
{
	cout <<"usage: " <<name <<" [options] hostname" <<endl <<endl
		 <<"where hostname is the name of the host to connect," <<endl
		 <<"and options can be one of:" <<endl
		 <<"\t-i port   uses the port specified for" <<endl
		 <<"\t          the input messages connection (default 4321)" <<endl
		 <<"\t-s port   uses the port specified for" <<endl
		 <<"\t          the display connection (default 4322)" <<endl
		 <<"\t-p pass   connect using this password" <<endl
		 <<"\t-h        displays this message" <<endl;
	Quit();
}

status_t ClientApp::parse(int argc, char **argv)
{
	int c;
	extern int optind;
	extern char *optarg;
	
	while ((c = getopt(argc, argv, "p:i:hs:")) != EOF)
		switch (c)
		{
			case 'p':
				mPasswd=optarg;
				break;
			case 'i':
				mPorti=atoi(optarg);
				break;
			case 's':
				mPorts=atoi(optarg);
				break;
			case 'h':
			case '?':
				usage(argv[0]);
				return B_ERROR;
				break;
		}
	if(argc-optind!=1)
	{
		usage(argv[0]);
		return B_ERROR;
	}
	mHostname=argv[optind];
	return B_OK;
}

void ClientApp::ArgvReceived(int32 argc, char **argv)
{
	if(mArgsReceived)
		return;
	mArgsReceived=true;
	
	if(parse(argc, argv)!=B_OK)
		return;
	
	int32 socki=Connect(mPorti, mHostname);
	int32 socks=Connect(mPorts, mHostname);
	
	if((socki<0) || (socks<0))
	{
		cerr <<"couldn't connect to " <<mHostname <<endl;
		Quit();
		return;
	}
	
	mScreenClient=new ScreenClient(new ClientIO(socks), mPasswd);
	if(mScreenClient->InitCheck()!=B_OK)
	{
		delete mScreenClient;
		mScreenClient=0;
		Quit();
	}
	
	mInputClient=new InputClient(new ClientIO(socki), mPasswd);
	if(mInputClient->InitCheck()!=B_OK)
	{
		delete mInputClient;
		mInputClient=0;
		Quit();
	}
}

void ClientApp::ReadyToRun()
{
	if(!mArgsReceived)
	{
		usage("RCClient");
		return;
	}
	
	mWin=new ClientWin(mScreenClient->GetScreenFrame(), mInputClient, mScreenClient);
	mWin->Show();
	
	mControlWin=new ControlWindow(mWin, mInputClient, mScreenClient);
	mControlWin->Show();
}

bool ClientApp::QuitRequested()
{
	if(mScreenClient)
		mScreenClient->Stop();
	if(mWin->Lock())
		mWin->Quit();
	if(mControlWin->Lock())
		mControlWin->Quit();
	return true;
}

int32 ClientApp::Connect(int32 port, const char *server)
{
	struct sockaddr_in sa;
	int sock;
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		cerr <<"couldn't create socket -- " <<strerror(errno) <<endl;
		return errno;
	}
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = INADDR_ANY;
	memset(sa.sin_zero, 0, sizeof(sa.sin_zero));
	
	if(server)
	{
		hostent *ent=gethostbyname(server);
		if(!ent)
			return B_ERROR;
		memcpy( (char*)&sa.sin_addr, ent->h_addr, ent->h_length );
	}
	
	if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0)
	{
		cerr <<"couldn't connect -- " <<strerror(errno) <<endl;
		return errno;
	}
	return sock;
}
