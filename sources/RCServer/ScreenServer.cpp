#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include <Debug.h>
#include <Autolock.h>
#include <View.h>
#include <BitmapStream.h>
#include <TranslatorRoster.h>
#include <Application.h>
#include <Roster.h>
#include <Bitmap.h>
#include <Screen.h>

#include "autodelete.h"
#include "ClientIO.h"
#include "ScreenServer.h"
#include "proto.h"
#include "Preferences.h"
#include "Password.h"

ScreenServer::ScreenServer(ClientIO *clio, int32 address, char const *passwd) :
	mClio(clio),
	mThid(B_ERROR),
	mRecvThid(B_ERROR),
	mDeleted(false),
	mType(0),
	mUpdateActiveWindow(false),
	mUpdateChanged(true),
	mScreenBitmap(0),
	mAddress(address),
	mPasswd(passwd)
{
	char timestr[256];
	time_t timet=time(0);
	strftime(timestr, 256, "%c", localtime(&timet));
	cout <<timestr <<'\t';
	
	int32 ip=mAddress;
	for(int32 i=0; i<4; i++)
	{
		if(i)
			cout <<'.';
		cout <<(ip&0xff);
		ip>>=8;
	}
	cout <<" connected" <<endl;
	
	BMessage prefsMsg;
	Preferences::Instance()->Load(&prefsMsg, mAddress);
	
	prefsMsg.FindInt32("ss_type", &mType);
	if(prefsMsg.FindInt32("ss_square_size", &mSquareSize)!=B_OK)
		mSquareSize=100;
	if(prefsMsg.FindBool("ss_update_active", &mUpdateActiveWindow)!=B_OK)
		mUpdateActiveWindow=false;
	if(prefsMsg.FindBool("ss_update_changed", &mUpdateChanged)!=B_OK)
		mUpdateChanged=true;
	if(prefsMsg.FindBool("ss_send_diff", &mSendDiff)!=B_OK)
		mSendDiff=false;
	
	mRecvThid=spawn_thread(recvThread, "screen server recv", B_NORMAL_PRIORITY, this);
	if(mRecvThid<0)
	{
		cerr <<"couldn't spawn thread -- " <<strerror(mRecvThid) <<endl;
		delete this;
		return;
	}
	if(resume_thread(mRecvThid)!=B_OK)
	{
		kill_thread(mRecvThid);
		delete this;
		return;
	}
}

ScreenServer::~ScreenServer()
{
	char timestr[256];
	time_t timet=time(0);
	strftime(timestr, 256, "%c", localtime(&timet));
	cout <<timestr <<'\t';
	
	int32 ip=mAddress;
	for(int32 i=0; i<4; i++)
	{
		if(i)
			cout <<'.';
		cout <<(ip&0xff);
		ip>>=8;
	}
	cout <<" disconnected" <<endl;
	
	mClio->CloseSocket();
	status_t ret;
	mDeleted=true;
	if(mThid>0)
		wait_for_thread(mThid, &ret);
	if(mRecvThid>0)
		wait_for_thread(mRecvThid, &ret);
	if(mScreenBitmap)
		delete mScreenBitmap;
	delete mClio;
}

void ScreenServer::SendTranslators()
{
	BTranslatorRoster *r = BTranslatorRoster::Default();
	
	translator_id * list = NULL;
	int32 count = 0;
	
	{
		char *str="Raw data";
		int32 len=strlen(str);
		mClio->WriteBytes(&len, sizeof(int32));
		mClio->WriteBytes(str, len);
		len=0;
		mClio->WriteBytes(&len, sizeof(int32));
	}
	
	status_t err = r->GetAllTranslators(&list, &count);
	if (err < B_OK)
	{
		cerr <<"couldn't get translators -- " <<strerror(err) <<endl;
		int32 zero=0;
		mClio->WriteBytes(&zero, sizeof(int32));
		mClio->Flush();
		return;
	}
	if(!count)
	{
		cerr <<"no translators" <<endl;
		delete[] list;
		int32 zero=0;
		mClio->WriteBytes(&zero, sizeof(int32));
		mClio->Flush();
		return;
	}
	
	for (int tix=0; tix<count; tix++)
	{
		const char * name;
		const char * info;
		int32 version;
		if ((err = r->GetTranslatorInfo(list[tix], &name, &info, &version)) < B_OK)
			continue;
		int32 num_input = 0;
		const translation_format * input_formats = NULL;
		if ((err = r->GetInputFormats(list[tix], &input_formats, &num_input)) < B_OK)
			continue;
		for (int iix=0; iix<num_input; iix++)
			if ((input_formats[iix].type != input_formats[iix].group) && (input_formats[iix].group==B_TRANSLATOR_BITMAP)/*B_TRANSLATOR_BITMAP*/)
			{
				int32 len=strlen(name);
				if(!len)
					continue;
				mClio->WriteBytes(&len, sizeof(int32));
				mClio->WriteBytes(name, len);
				mClio->WriteBytes(&(input_formats[iix].type), sizeof(int32));
//				cout <<'"' <<name <<"\": " <<input_formats[iix].type <<endl;
				break;
			}
	}
	
	int32 zero=0;
	mClio->WriteBytes(&zero, sizeof(int32));
	mClio->Flush();
	
	delete[] list;
}

void ScreenServer::SendPrefs()
{
	BMessage prefsMsg;
	int32 size;
	if(Preferences::Instance()->Load(&prefsMsg, mAddress)==B_ERROR)
	{
		size=0;
		mClio->WriteBytes(&size, sizeof(int32));
		return;
	}
	size=prefsMsg.FlattenedSize();
	char *flat=new char[size];
	prefsMsg.Flatten(flat, size);
	mClio->WriteBytes(&size, sizeof(int32));
	mClio->WriteBytes(flat, size);
	delete flat;
}

BBitmap *ScreenServer::GetBitmap(BRect frame)
{
	BBitmap *bmp;
	
	BScreen screen;
	status_t err=screen.GetBitmap(&bmp, false, &frame);
	if(err!=B_OK)
	{
		cerr <<"couldn't get bitmap -- " <<strerror(err) <<endl;
		mClio->WriteBytes(&RCMessage::CouldntGetBitmap, sizeof(int32));
		mClio->Flush();
		return 0;
	}
	return bmp;
}

status_t ScreenServer::SendBitmap(BRect frame, BBitmap *bmp, bool diff)
{
	BBitmapStream stream(bmp);
	BTranslatorRoster *roster=BTranslatorRoster::Default();
	BMallocIO mallocio;
	mallocio.SetBlockSize(16384);
	
	mLock.Lock();
	int32 type=mType;
	mLock.Unlock();
	
	int32 size;
	
	if(type)
	{
		status_t err=roster->Translate(&stream, 0, 0, &mallocio, type);
		if(err!=B_OK)
		{
			cerr <<"couldn't translate -- " <<strerror(err) <<endl;
			mClio->WriteBytes(&RCMessage::CouldntTranslate, sizeof(int32));
			mClio->Flush();
			return err;
		}
		size=mallocio.BufferLength();
		mClio->WriteBytes(&RCMessage::BitmapFollowing, sizeof(int32));
		mClio->WriteBytes(&frame, sizeof(frame));
		mClio->WriteBytes(&diff, sizeof(bool));
		mClio->WriteBytes(&size, sizeof(int32));
		mClio->WriteBytes(mallocio.Buffer(), size);
	}
	else
	{
		size=stream.Size();
		mClio->WriteBytes(&RCMessage::BitmapFollowing, sizeof(int32));
		mClio->WriteBytes(&frame, sizeof(frame));
		mClio->WriteBytes(&diff, sizeof(bool));
		mClio->WriteBytes(&size, sizeof(int32));
		
		char *buffer=new char[size];
		autodelete<char> deleteBuffer(buffer);
		stream.Seek(0, SEEK_SET);
		
		char *oldBuffer=buffer;
		int32 oldSize=size;
		
		do
		{
			int32 s=stream.Read(buffer, size);
			size-=s;
			buffer+=s;
		} while(size);
		
		mClio->WriteBytes(oldBuffer, oldSize);
	}
	
	mClio->Flush();
	
	return B_OK;
}

int32 ScreenServer::CompareBitmaps(BBitmap *bmp1, BBitmap *bmp2, BRect frame, BBitmap **diffBmp, BRect *diffFrame)
{
	char *bits1=(char *)bmp1->Bits();
	const char *bits2=(char *)bmp2->Bits();
	int32 bpr1=bmp1->BytesPerRow();
	int32 bpr2=bmp2->BytesPerRow();
	int32 height=int32(frame.Height()+1);
	int32 offset=int32(frame.left)*4+int32(frame.top)*bpr2;
	
	int32 boundX0=0;
	int32 boundX1=(int32)frame.Width();
	int32 boundY0=0;
	int32 boundY1=(int32)frame.Height();
	
	bool test;
	
	test=false;
	for(int32 i=0; i<height; i++)
	{
		if(memcmp(&(bits1[i*bpr1]), &(bits2[offset+i*bpr2]), bpr1))
		{
			boundY0=i;
			test=true;
			break;
		}
	}
	if(!test)
		return 0;
	
	test=false;
	for(int32 i=height-1; i>=0; i--)
	{
		if(memcmp(&(bits1[i*bpr1]), &(bits2[offset+i*bpr2]), bpr1))
		{
			boundY1=i;
			test=true;
			break;
		}
	}
	ASSERT(test);
	
	test=false;
	for(int32 j=0; j<bpr1; j++)
	{
		for(int32 i=boundY0; i<=boundY1; i++)
			if(bits1[i*bpr1+j]!=bits2[offset+i*bpr2+j])
			{
				boundX0=j/4;
				test=true;
				break;
			}
		if(test)
			break;
	}
	ASSERT(test);
	
	test=false;
	for(int32 j=bpr1-1; j>=0; j--)
	{
		for(int32 i=boundY0; i<=boundY1; i++)
			if(bits1[i*bpr1+j]!=bits2[offset+i*bpr2+j])
			{
				boundX1=j/4;
				test=true;
				break;
			}
		if(test)
			break;
	}
	ASSERT(test);
	
	diffFrame->right =frame.left+boundX1;
	diffFrame->bottom=frame.top+boundY1;
	diffFrame->left  =frame.left+boundX0;
	diffFrame->top   =frame.top+boundY0;
	
	*diffBmp=new BBitmap(BRect(0, 0, boundX1-boundX0, boundY1-boundY0), B_RGB32);
	
	bits2=bits1;
	bpr2=bpr1;
	bits1=(char *)(*diffBmp)->Bits();
	bpr1=(*diffBmp)->BytesPerRow();
	height=int32(diffFrame->Height()+1);
	offset=int32(boundX0)*4+int32(boundY0)*bpr2;
	for(int32 i=0; i<height; i++)
		memcpy(&(bits1[i*bpr1]), &(bits2[offset+i*bpr2]), bpr1);
	
	return 1;
}

void ScreenServer::SubstractBitmap(BBitmap *bmp1, BBitmap *bmp2, BRect frame)
{
	char *bits1=(char *)bmp1->Bits();
	const char *bits2=(char *)bmp2->Bits();
	int32 bpr1=bmp1->BytesPerRow();
	int32 bpr2=bmp2->BytesPerRow();
	int32 height=int32(frame.Height()+1);
	int32 offset=int32(frame.left)*4+int32(frame.top)*bpr2;
	for(int32 i=0; i<height; i++)
		for(int32 j=0; j<bpr1; j++)
			bits1[i*bpr1+j]-=bits2[offset+i*bpr2+j];
}

void ScreenServer::AddBitmap(BBitmap *bmp1, BRect frame, BBitmap *bmp2)
{
	char *bits1=(char *)bmp1->Bits();
	const char *bits2=(char *)bmp2->Bits();
	int32 bpr1=bmp1->BytesPerRow();
	int32 bpr2=bmp2->BytesPerRow();
	int32 height=int32(frame.Height()+1);
	int32 offset=int32(frame.left)*4+int32(frame.top)*bpr1;
	for(int32 i=0; i<height; i++)
		for(int32 j=0; j<bpr2; j++)
			bits1[offset+i*bpr1+j]+=bits2[i*bpr2+j];
}

void ScreenServer::CopyBitmap(BBitmap *bmp1, BRect frame, BBitmap *bmp2)
{
	char *bits1=(char *)bmp1->Bits();
	const char *bits2=(char *)bmp2->Bits();
	int32 bpr1=bmp1->BytesPerRow();
	int32 bpr2=bmp2->BytesPerRow();
	int32 height=int32(frame.Height()+1);
	int32 offset=int32(frame.left)*4+int32(frame.top)*bpr1;
	for(int32 i=0; i<height; i++)
		memcpy(&(bits1[offset+i*bpr1]), &(bits2[i*bpr2]), bpr2);
}

int32 ScreenServer::CountSquares(BRect frame)
{
	BAutolock autolocker(mLock);
	return (int32(frame.Width()+mSquareSize)/mSquareSize)*(int32(frame.Height()+mSquareSize)/mSquareSize);
}

BRect ScreenServer::GetSquare(BRect frame, int32 num)
{
	BAutolock autolocker(mLock);
	int32 squaresByLine=int32(frame.Width()+mSquareSize)/mSquareSize;
	return BRect(frame.left+mSquareSize*(num%squaresByLine),
				 frame.top +mSquareSize*(num/squaresByLine),
				 min_c(frame.left+mSquareSize*(1+num%squaresByLine)-1, frame.right),
				 min_c(frame.top +mSquareSize*(1+num/squaresByLine)-1, frame.bottom));
}

int32 ScreenServer::sendBitmapThread(void *cookie)
{
	ScreenServer *ptr=((ScreenServer *)cookie);
	status_t err=B_OK;
	
	try
	{
		err=ptr->SendBitmapThread();
	}
	catch(ClientDisconnected)
	{
	}
	
	ptr->mThid=B_ERROR;
	ptr->mLock.Lock();
	if(!ptr->mDeleted)
	{
		ptr->mDeleted=true;
		ptr->mLock.Unlock();
		delete ptr;
	}
	
	return err;
}

int32 ScreenServer::SendBitmapThread()
{
	BScreen screen;
	
	BRect screenFrame=screen.Frame();
	mClio->WriteBytes(&screenFrame, sizeof(screenFrame));
	
	SendTranslators();
	SendPrefs();
	
	mScreenBitmap=new BBitmap(screenFrame, B_RGB32);
	memset(mScreenBitmap->Bits(), 0, mScreenBitmap->BitsLength());
	
	int32 cptr=0;
	
	while(!mDeleted)
	{
		BRect frame;
		if(!mUpdateActiveWindow)
			frame=screenFrame;
		else
			if(GetActiveWindowFrame(&frame)!=B_OK)
				frame=screenFrame;
			else
			{
				frame=frame & screenFrame;
				if((frame.Width()<0) || (frame.Height()<0))
					continue;
			}
		cptr=cptr%CountSquares(frame);
		BRect square=GetSquare(frame, cptr);
		BBitmap *bmp=GetBitmap(square);
		
		bool updateChanged=mUpdateChanged;
		
		if(bmp)
		{
			BBitmap *diffBmp;
			BRect diffFrame;
			if(!updateChanged || CompareBitmaps(bmp, mScreenBitmap, square, &diffBmp, &diffFrame))
			{
				if(!updateChanged)
				{
					diffFrame=square;
					diffBmp=bmp;
				}
				bool diff=mSendDiff;
				if(diff)
				{
					SubstractBitmap(diffBmp, mScreenBitmap, diffFrame);
					AddBitmap(mScreenBitmap, diffFrame, diffBmp);
				}
				else
					CopyBitmap(mScreenBitmap, diffFrame, diffBmp);
				SendBitmap(diffFrame, diffBmp, diff);
				if(updateChanged)
					delete bmp;
			}
			else
				delete bmp;
		}
		cptr++;
	}
	
	return B_OK;
}

int32 ScreenServer::recvThread(void *cookie)
{
	ScreenServer *ptr=((ScreenServer *)cookie);
	status_t err=B_OK;
	
	try
	{
		err=ptr->RecvThread();
	}
	catch(ClientDisconnected)
	{
	}
	
	ptr->mRecvThid=B_ERROR;
	ptr->mLock.Lock();
	if(!ptr->mDeleted)
	{
		ptr->mDeleted=true;
		ptr->mLock.Unlock();
		delete ptr;
	}
	return err;
}

int32 ScreenServer::RecvThread()
{
	status_t err;
	
	if(!Password(mClio).RecvPassword(mPasswd))
	{
		char timestr[256];
		time_t timet=time(0);
		strftime(timestr, 256, "%c", localtime(&timet));
		cout <<timestr <<'\t';
		
		int32 ip=mAddress;
		for(int32 i=0; i<4; i++)
		{
			if(i)
				cout <<'.';
			cout <<(ip&0xff);
			ip>>=8;
		}
		cout <<" bad password" <<endl;
		return B_ERROR;
	}
	
	mThid=spawn_thread(sendBitmapThread, "screen server send", B_NORMAL_PRIORITY, this);
	if(mThid<0)
	{
		cerr <<"couldn't spawn thread -- " <<strerror(mThid) <<endl;
		return B_ERROR;
	}
	if(resume_thread(mThid)!=B_OK)
	{
		kill_thread(mThid);
		return B_ERROR;
	}
	
	while(1)
	{
		int32 msg;
		mClio->ReadBytes(&msg, sizeof(int32));
		
		Preferences *prefs=Preferences::Instance();
		prefs->Lock();
		BMessage prefsMsg;
		prefs->Load(&prefsMsg, mAddress);
		
		switch(msg)
		{
			case RCMessage::ChangeTranslator:
			{
				int32 type;
				
				mClio->ReadBytes(&type, sizeof(int32));
				
				mLock.Lock();
				mType=type;
				mLock.Unlock();
				if((prefsMsg.ReplaceInt32("ss_type", type)!=B_OK) &&
					(err=prefsMsg.AddInt32("ss_type", type)!=B_OK))
					cout <<strerror(err) <<endl;
				break;
			}
			case RCMessage::UpdateActiveWindow:
				mUpdateActiveWindow=true;
				if((prefsMsg.ReplaceBool("ss_update_active", mUpdateActiveWindow)!=B_OK) &&
					(err=prefsMsg.AddBool("ss_update_active", mUpdateActiveWindow)!=B_OK))
					cout <<strerror(err) <<endl;
				break;
			case RCMessage::UpadteAll:
				mUpdateActiveWindow=false;
				if((prefsMsg.ReplaceBool("ss_update_active", mUpdateActiveWindow)!=B_OK) &&
					(err=prefsMsg.AddBool("ss_update_active", mUpdateActiveWindow)!=B_OK))
					cout <<strerror(err) <<endl;
				break;
			case RCMessage::UpadteChanged:
				mUpdateChanged=true;
				if((prefsMsg.ReplaceBool("ss_update_changed", mUpdateChanged)!=B_OK) &&
					(err=prefsMsg.AddBool("ss_update_changed", mUpdateChanged)!=B_OK))
					cout <<strerror(err) <<endl;
				break;
			case RCMessage::UpadteEverything:
				mUpdateChanged=false;
				if((prefsMsg.ReplaceBool("ss_update_changed", mUpdateChanged)!=B_OK) &&
					(err=prefsMsg.AddBool("ss_update_changed", mUpdateChanged)!=B_OK))
					cout <<strerror(err) <<endl;
				break;
			case RCMessage::SetSquareSize:
				mLock.Lock();
				mClio->ReadBytes(&mSquareSize, sizeof(int32));
				mLock.Unlock();
				if((prefsMsg.ReplaceInt32("ss_square_size", mSquareSize)!=B_OK) &&
					(err=prefsMsg.AddInt32("ss_square_size", mSquareSize)!=B_OK))
					cout <<strerror(err) <<endl;
				break;
			case RCMessage::SendDiff:
				mClio->ReadBytes(&mSendDiff, sizeof(bool));
				if((prefsMsg.ReplaceBool("ss_send_diff", mSendDiff)!=B_OK) &&
					(err=prefsMsg.AddBool("ss_send_diff", mSendDiff)!=B_OK))
					cout <<strerror(err) <<endl;
				break;
		}
		
		prefs->Save(&prefsMsg, mAddress);
		prefs->Unlock();
	}
	
	return B_OK;
}

const bigtime_t HUGE_TIMEOUT = 1000000;

status_t ScreenServer::GetActiveWindowFrame(BRect *frame)
{
	app_info info;
	if(be_roster->GetActiveAppInfo(&info)!=B_OK)
		return B_ERROR;
	
	BMessenger msgr(0, info.team);
	BMessage reply;
	int32 wincount;
	
	BMessage count_msg(B_COUNT_PROPERTIES);
	count_msg.AddSpecifier("Window");
	if(msgr.SendMessage(&count_msg, &reply, HUGE_TIMEOUT, HUGE_TIMEOUT)!=B_OK)
		return B_ERROR;
	if(reply.FindInt32("result", 0, &wincount)!=B_OK)
		return B_ERROR;
	for(int i=0; i<wincount; i++)
	{
		bool active;
		BMessage active_msg(B_GET_PROPERTY);
		active_msg.AddSpecifier("Active");
		active_msg.AddSpecifier("Window", i);
		if(msgr.SendMessage(&active_msg, &reply, HUGE_TIMEOUT, HUGE_TIMEOUT)!=B_OK)
			continue;
		if(reply.FindBool("result", 0, &active)!=B_OK)
			continue;
		if(active)
		{
			BMessage frame_msg(B_GET_PROPERTY);
			frame_msg.AddSpecifier("Frame");
			frame_msg.AddSpecifier("Window", i);
			if(msgr.SendMessage(&frame_msg, &reply, HUGE_TIMEOUT, HUGE_TIMEOUT)!=B_OK)
				return B_ERROR;
			if(reply.FindRect("result", 0, frame)!=B_OK)
				return B_ERROR;
			return B_OK;
		}
	}
	return B_ERROR;
}
