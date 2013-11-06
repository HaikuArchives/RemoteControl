#include "PixelRateCalculator.h"

PixelRateCalculator::PixelRateCalculator(BMessage *msg, BMessenger msngr, int32 meanSize) :
	BInvoker(msg, msngr),
	mPos(0),
	mTotalPixels(0),
	mMeanSize(meanSize)
{
	if(mMeanSize<1)
		mMeanSize=1;
	mMeanBuffer=new PixelsData[mMeanSize];
	bigtime_t now=real_time_clock_usecs();
	mLastTime=now+1;
	for(int32 i=0; i<mMeanSize; i++)
	{
		mMeanBuffer[i].pixels=0;
		mMeanBuffer[i].when=now;
	}
}

PixelRateCalculator::~PixelRateCalculator()
{
}

void PixelRateCalculator::AddPixels(int32 amount)
{
	mLock.Lock();
	mTotalPixels=mTotalPixels-mMeanBuffer[mPos].pixels+amount;
	mMeanBuffer[mPos].pixels=amount;
	mMeanBuffer[mPos].when  =real_time_clock_usecs();
	mLastTime=mMeanBuffer[mPos].when;
	mPos++;
	mPos%=mMeanSize;
	mLock.Unlock();
	InvokeNotify(Message());
}

double PixelRateCalculator::GetRate()
{
	mLock.Lock();
	double rate=1000000.0*double(mTotalPixels)/double(mLastTime-mMeanBuffer[mPos].when)*(double(mMeanSize)/double(mMeanSize+1));
	mLock.Unlock();
	return rate;
}
