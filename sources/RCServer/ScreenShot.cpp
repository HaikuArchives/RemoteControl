#include <iostream>
#include <string.h>

#include <Bitmap.h>

#include "ScreenShot.h"

ScreenShot::ScreenShot() :
	mShot(0)
{
	ppus=new double[100];
	for(int32 i=0; i<100; i++)
		ppus[i]=0;
	cptr=-3;
}

ScreenShot::~ScreenShot()
{
}

void ScreenShot::Refresh(bool draw_cursor)
{
//	snooze(1000000);
	
	if(mShot)
		delete mShot;
	
	status_t err2=mScreen.GetBitmap(&mShot, draw_cursor);
	if(err2!=B_OK)
	{
		cerr <<"couldn't get bitmap -- " <<strerror(err2) <<endl;
		mShot=0;
		return;
	}
	
	for(int32 i=0; i<50; i++)
	{
		bigtime_t t1, t2;
		BRect r=BRect(0, 0, 10*i, 500);
		t1=real_time_clock_usecs();
		status_t err=mScreen.ReadBitmap(mShot, draw_cursor, &r);
		t2=real_time_clock_usecs();
		if(err!=B_OK)
		{
			cerr <<"couldn't get bitmap -- " <<strerror(err) <<endl;
			continue;
		}
//		else
//			delete mShot;
		int32 nbpix=5000*i;
		double tmpppus=((double)nbpix)/(double)(t2-t1);
		cout <<"pixels: " <<nbpix <<endl;//<<"\ttime: " <<t2-t1 <<"\tppus: " <<tmpppus;
		if(cptr<=0)
			ppus[i]=tmpppus;
		else
			ppus[i]=(cptr*ppus[i]+tmpppus)/(cptr+1);
		cout /*<<"\tmeanppus: "*/ <<ppus[i] <<endl;
	}
	cout <<endl;
	cptr++;
	
	if(mShot)
		delete mShot;
	
	status_t err=mScreen.GetBitmap(&mShot, draw_cursor);
	if(err!=B_OK)
	{
		cerr <<"couldn't get bitmap -- " <<strerror(err) <<endl;
		mShot=0;
	}
}

status_t ScreenShot::ReadBitmap(BBitmap *buffer, BRect *bounds=0)
{
	if(!mShot)
		return B_ERROR;
	BRect b;
	if(!bounds)
		b=mShot->Bounds();
	else
		b=*bounds;
	if(!buffer->Bounds().Contains(b))
		return B_ERROR;
	if(mShot->ColorSpace()!=buffer->ColorSpace())
		return B_ERROR;
	
	char *bits1=(char *)mShot->Bits();
	char *bits2=(char *)buffer->Bits();
	int32 bpr1=mShot->BytesPerRow();
	int32 bpr2=buffer->BytesPerRow();
	int32 bytesPerPixel=bpr1/(int32)(mShot->Bounds().Width()+1);
	
	for(int32 line=0; line<b.Height()+1; line++)
		memcpy(&(bits2[line*bpr2]), &(bits1[(line+(int32)b.top)*bpr1+((int32)b.left)*bytesPerPixel]), ((int32)b.Width()+1)*bytesPerPixel);
	
	return B_OK;
}

status_t ScreenShot::GetBitmap(BBitmap **buffer, BRect *bounds=0)
{
	if((!buffer) || (!mShot))
		return B_ERROR;
	BRect b;
	if(!bounds)
		b=mShot->Bounds();
	else
		b=*bounds;
	*buffer=new BBitmap(*bounds, mShot->ColorSpace());
	return ReadBitmap(*buffer, &b);
}
