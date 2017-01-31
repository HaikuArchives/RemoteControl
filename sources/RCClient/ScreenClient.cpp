#include <iostream>
#include <string.h>
#include <sys/socket.h>

#include <Alert.h>
#include <BitmapStream.h>
#include <TranslatorRoster.h>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Bitmap.h>
#include <Screen.h>

#include "PixelRateCalculator.h"
#include "autodelete.h"
#include "ClientIO.h"
#include "ClientView.h"
#include "ScreenClient.h"
#include "proto.h"
#include "Password.h"

using namespace std;

ScreenClient::ScreenClient(ClientIO *clio, char const *passwd) :
	mClio(clio),
	mQuit(false),
	mTransData(0),
	mPixelRateCalculator(0),
	mPrefs(0)
{
	mInit=B_OK;
	if(!Password(mClio).SendPassword(passwd))
	{
		if(passwd)
			cout <<"bad password" <<endl;
		else
			cout <<"password required" <<endl;
		mInit=B_ERROR;
		return;
	}
	
	mPort=find_port(PortName);
	if(mPort<0)
	{
		cerr <<"couldn't find port -- " <<strerror(mPort) <<endl;
		return;
	}
	
	mThid=spawn_thread(threadFunc, "screen client", B_NORMAL_PRIORITY, this);
	if(mThid<0)
	{
		cerr <<"couldn't spawn thread -- " <<strerror(mThid) <<endl;
		return;
	}
	
	mScreenFrame=ReceiveFrame();
	ReceiveTranslators();
	ReceivePrefs();
}

ScreenClient::~ScreenClient()
{
	mQuit=true;
	mClio->CloseSocket();
	status_t ret;
	if(mThid>0)
		wait_for_thread(mThid, &ret);
	if(mTransData)
	{
		for(int32 i=0; i<mNumTrans; i++)
			delete mTransData[i].name;
		delete mTransData;
	}
	delete mClio;
}

status_t ScreenClient::InitCheck()
{
	return mInit;
}

void ScreenClient::Stop()
{
	if(mThid<0)
		return;
	mQuit=true;
	mClio->CloseSocket();
	status_t ret;
	wait_for_thread(mThid, &ret);
	mThid=-1;
}

void ScreenClient::ChangeTranslator(int32 type, int32 id)
{
	mClio->WriteBytes(&RCMessage::ChangeTranslator, sizeof(int32));
	mClio->WriteBytes(&type, sizeof(int32));
	mClio->Flush();
}

void ScreenClient::UpdateActiveWindow()
{
	mClio->WriteBytes(&RCMessage::UpdateActiveWindow, sizeof(int32));
	mClio->Flush();
}

void ScreenClient::UpadteAll()
{
	mClio->WriteBytes(&RCMessage::UpadteAll, sizeof(int32));
	mClio->Flush();
}

void ScreenClient::UpdateChanged()
{
	mClio->WriteBytes(&RCMessage::UpadteChanged, sizeof(int32));
	mClio->Flush();
}

void ScreenClient::UpadteEverything()
{
	mClio->WriteBytes(&RCMessage::UpadteEverything, sizeof(int32));
	mClio->Flush();
}

void ScreenClient::SetSquareSize(int32 size)
{
	mClio->WriteBytes(&RCMessage::SetSquareSize, sizeof(int32));
	mClio->WriteBytes(&size, sizeof(int32));
	mClio->Flush();
}

void ScreenClient::SendDiff(bool sendDiff)
{
	mClio->WriteBytes(&RCMessage::SendDiff, sizeof(int32));
	mClio->WriteBytes(&sendDiff, sizeof(bool));
	mClio->Flush();
}

void ScreenClient::SetView(ClientView *view)
{
	mView=view;
	resume_thread(mThid);
}

void ScreenClient::SetPixelRateCalculator(PixelRateCalculator *calc)
{
	mPixelRateCalculator=calc;
}

BRect ScreenClient::ReceiveFrame()
{
	BRect frame;
	
	mClio->ReadBytes(&frame, sizeof(frame));
	return frame;
}

void ScreenClient::ReceiveTranslators()
{
	mNumTrans=0;
	
	int32 len;
	mClio->ReadBytes(&len, sizeof(int32));
	
	while(len)
	{
		char *name=new char[len+1];
		mClio->ReadBytes(name, len);
		name[len]=0;
		int32 type;
		mClio->ReadBytes(&type, sizeof(int32));
		
		TranslatorData *newTransData=new TranslatorData[mNumTrans+1];
		memcpy(newTransData, mTransData, mNumTrans*sizeof(TranslatorData));
		delete mTransData;
		mTransData=newTransData;
		
		mTransData[mNumTrans].type=type;
		mTransData[mNumTrans].id  =0;
		mTransData[mNumTrans].name=name;
		mNumTrans++;
		
		mClio->ReadBytes(&len, sizeof(int32));
	}
}

void ScreenClient::ReceivePrefs()
{
	int32 size;
	mClio->ReadBytes(&size, sizeof(int32));
	
	if(size<=0)
		return;
	
	char *flat=new char[size];
	mClio->ReadBytes(flat, size);
	BMessage *msg=new BMessage;
	if(msg->Unflatten(flat)==B_OK)
	{
		if(mPrefs)
			delete mPrefs;
		mPrefs=msg;
	}
	else
		delete msg;
	
	delete flat;
}

BBitmap *ScreenClient::ReceiveBitmap()
{
	int32 bmpSize;
	
	mClio->ReadBytes(&bmpSize, sizeof(int32));
	
	char *buffer=new char[bmpSize];
	autodelete<char> deleteBuffer(buffer);
	
	mClio->ReadBytes(buffer, bmpSize);
	BMemoryIO memio(buffer, bmpSize);
	
	BBitmapStream stream;
	BTranslatorRoster *roster=BTranslatorRoster::Default();
	
	status_t err=roster->Translate(&memio, 0, 0, &stream, B_TRANSLATOR_BITMAP);
	if(err!=B_OK)
	{
		cout <<"couldn't translate -- " <<strerror(err) <<endl;
		return 0;
	}
	
	BBitmap *bmp;
	stream.DetachBitmap(&bmp);
	
	if(mPixelRateCalculator)
		mPixelRateCalculator->AddPixels((bmp->Bounds().Width()+1)*(bmp->Bounds().Height()+1));
	
	return bmp;
}

int32 ScreenClient::threadFunc(void *cookie)
{
	ScreenClient *ptr=(ScreenClient *)cookie;
	try
	{
		return ptr->ThreadFunc();
	}
	catch (ClientDisconnected)
	{
		if(!ptr->mQuit)
			(new BAlert("Disconnected", "Connection broken", "Quit", 0, 0, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		return B_OK;
	}
}

int32 ScreenClient::ThreadFunc()
{
	while(!mQuit)
	{
		int32 code;
		mClio->ReadBytes(&code, sizeof(int32));
		if(code!=RCMessage::BitmapFollowing)
			continue;
		BRect frame=ReceiveFrame();
		bool diff;
		mClio->ReadBytes(&diff, sizeof(bool));
		BBitmap *bmp=ReceiveBitmap();
		if(bmp)
		{
			mView->CopyBitmap(bmp, frame, diff);
			delete bmp;
		}
	}
	
	return B_OK;
}
