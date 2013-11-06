#include "Password.h"
#include "ClientIO.h"
#include "proto.h"

Password::Password(ClientIO *clio) :
	mClio(clio)
{
}

Password::~Password()
{
}

bool Password::SendPassword(char const *passwd)
{
	int32 msg;
	mClio->ReadBytes(&msg, sizeof(int32));
	if(msg==RCMessage::NoPassword)
		return true;
	if(!passwd)
		return false;
	int32 len=strlen(passwd);
	mClio->WriteBytes(&len, sizeof(int32));
	mClio->WriteBytes(passwd, len);
	mClio->Flush();
	mClio->ReadBytes(&msg, sizeof(int32));
	
	return (msg==RCMessage::PasswordOK);
}

bool Password::RecvPassword(char const *passwd)
{
	if(passwd)
	{
		mClio->WriteBytes(&RCMessage::RequirePassword, sizeof(int32));
		mClio->Flush();
		int32 len;
		mClio->ReadBytes(&len, sizeof(int32));
		char *pass=new char[len+1];
		mClio->ReadBytes(pass, len);
		pass[len]=0;
		if(strcmp(pass, passwd))
		{
			delete pass;
			mClio->WriteBytes(&RCMessage::BadPassword, sizeof(int32));
			mClio->Flush();
			return false;
		}
		delete pass;
		mClio->WriteBytes(&RCMessage::PasswordOK, sizeof(int32));
		mClio->Flush();
	}
	else
	{
		mClio->WriteBytes(&RCMessage::NoPassword, sizeof(int32));
		mClio->Flush();
	}
	
	return true;
}
