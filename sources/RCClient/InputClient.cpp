#include <iostream>
#include <string.h>

#include <Application.h>
#include <Alert.h>
#include <Message.h>
#include <Point.h>

#include "autodelete.h"
#include "InputClient.h"
#include "ClientIO.h"
#include "proto.h"
#include "Password.h"

InputClient::InputClient(ClientIO *clio, char const *passwd) :
	mClio(clio),
	mEnabled(false)
{
	mInit=B_OK;
	if(!Password(mClio).SendPassword(passwd))
		mInit=B_ERROR;
}

InputClient::~InputClient()
{
	delete mClio;
}

status_t InputClient::InitCheck()
{
	return mInit;
}

void InputClient::SendMessage(const BMessage *msg)
{
	if(!mEnabled)
		return;
	
	int32 size=msg->FlattenedSize();
	char *flat=new char[size];
	autodelete<char> deleteFlat(flat);
	msg->Flatten(flat, size);
	
	try
	{
		mClio->WriteBytes(&RCMessage::FlatMessage, sizeof(int32));
		mClio->WriteBytes(&size, sizeof(int32));
		mClio->WriteBytes(flat, size);
		mClio->Flush();
	}
	catch(ClientDisconnected)
	{
		(new BAlert("Disconnected", "Connection broken", "Quit", 0, 0, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	}
}

void InputClient::EnableFilter(bool enable=true)
{
	if(enable)
		mClio->WriteBytes(&RCMessage::EnableFilter, sizeof(int32));
	else
		mClio->WriteBytes(&RCMessage::DisableFilter, sizeof(int32));
	mClio->Flush();
}
