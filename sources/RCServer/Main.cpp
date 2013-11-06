#include <iostream.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>

#include <Application.h>
#include <OS.h>

#include "Preferences.h"
#include "MyApp.h"
#include "InputListener.h"
#include "ScreenListener.h"

struct
{
	int32 porti;
	int32 ports;
	char const *passwd;
} args;

void usage(char *name, int err=1)
{
	cout <<"usage: " <<name <<" [options]" <<endl <<endl
		 <<"where options can be one of:" <<endl
		 <<"\t-i port   uses the port specified for" <<endl
		 <<"\t          the input messages connection (default 4321)" <<endl
		 <<"\t-s port   uses the port specified for" <<endl
		 <<"\t          the display connection (default 4322)" <<endl
		 <<"\t-p pass   the clients require this passwd to connect" <<endl
		 <<"\t-h        displays this message" <<endl;
	exit(err);
}

void parse(int argc, char **argv)
{
	int c;
	extern int optind;
	extern char *optarg;
	
	while ((c = getopt(argc, argv, "p:i:hs:")) != EOF)
		switch (c)
		{
			case 'p':
				args.passwd=optarg;
				break;
			case 'i':
				args.porti=atoi(optarg);
				break;
			case 'h':
				usage(argv[0], 0);
				break;
			case 's':
				args.ports=atoi(optarg);
				break;
			case '?':
				usage(argv[0]);
				break;
		}
	if(argc-optind)
		usage(argv[0]);
}

int32 CreateSocket(int32 port)
{
	status_t err;
	struct sockaddr_in sa;
	int sock;
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		cerr <<"couldn't create socket -- " <<strerror(errno) <<endl;
		return sock;
	}
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = INADDR_ANY;
	memset(sa.sin_zero, 0, sizeof(sa.sin_zero));
	
	err=bind(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (err < 0)
	{
		cerr <<"couldn't bind -- " <<strerror(errno) <<endl;
		return err;
	}
	
	err=listen(sock, 1);
	if(err<0)
	{
		cerr <<"couldn't listen -- " <<strerror(errno) <<endl;
		return err;
	}
	
	return sock;
}

int main(int argc, char **argv)
{
	Preferences::Instance()->SetFile("/boot/home/config/settings/RemoteControl.perfs");
	
	args.porti=4321;
	args.ports=4322;
	args.passwd=0;
	
	parse(argc, argv);
	
	MyApp *app=new MyApp();
	
	int32 socki=CreateSocket(args.porti);
	int32 socks=CreateSocket(args.ports);
	
	if((socki<0) || (socks<0))
		return 1;
	
	new InputListener (socki, args.passwd);
	new ScreenListener(socks, args.passwd);
	
	app->Run();
	
	return 0;
}
