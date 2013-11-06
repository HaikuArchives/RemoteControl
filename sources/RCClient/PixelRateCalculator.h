#include <Invoker.h>
#include <Locker.h>

class PixelRateCalculator: public BInvoker
{
	public:
		PixelRateCalculator(BMessage *msg, BMessenger msngr, int32 meanSize=10);
		~PixelRateCalculator();
		
		void AddPixels(int32 amount);
		double GetRate();
		
	private:
		struct PixelsData
		{
			int32 pixels;
			bigtime_t when;
		};
		PixelsData *mMeanBuffer;
		
		bigtime_t mLastTime;
		BLocker mLock;
		int32 mPos;
		int32 mTotalPixels;
		int32 mMeanSize;
};
