#include <iostream>

#include <Application.h>

#include "ClientView.h"
#include "InputClient.h"
#include "ScreenClient.h"

#include "ClientWin.h"

ClientWin::ClientWin(BRect frame, InputClient *inputClient, ScreenClient *screenClient) :
	BWindow(frame, "Remote control", B_TITLED_WINDOW, 0),
	mInputClient(inputClient),
	mScreenClient(screenClient)
{
	mView=new ClientView(Bounds(), mInputClient, mScreenClient);
	AddChild(mView);
}

ClientWin::~ClientWin()
{
}

bool ClientWin::QuitRequested()
{
	mScreenClient->Stop();
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	return true;
}
