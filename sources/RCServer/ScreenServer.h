#include <Bitmap.h>
#include <Locker.h>
#include <OS.h>
#include <Rect.h>

class ClientIO;

class ScreenServer
{
	public:
		ScreenServer(ClientIO *clio, int32 address, char const *passwd);
		~ScreenServer();
		
	private:
		void SendTranslators();
		void SendPrefs();
		BBitmap *GetBitmap(BRect frame);
		status_t SendBitmap(BRect frame, BBitmap *bmp, bool diff);
		int32 CompareBitmaps(BBitmap *bmp1, BBitmap *bmp2, BRect frame, BBitmap **diffBmp, BRect *diffFrame);
		void SubstractBitmap(BBitmap *bmp1, BBitmap *bmp2, BRect frame);
		void AddBitmap(BBitmap *bmp1, BRect frame, BBitmap *bmp2);
		void CopyBitmap(BBitmap *bmp1, BRect frame, BBitmap *bmp2);
		int32 CountSquares(BRect frame);
		BRect GetSquare(BRect frame, int32 num);
		static int32 recvThread(void *cookie);
		int32 RecvThread();
		static int32 sendBitmapThread(void *cookie);
		int32 SendBitmapThread();
		status_t GetActiveWindowFrame(BRect *frame);
		
		ClientIO *mClio;
		thread_id mThid;
		thread_id mRecvThid;
		bool mDeleted;
		
		int32 mType;
		bool mUpdateActiveWindow;
		bool mUpdateChanged;
		int32 mSquareSize;
		bool mSendDiff;
		
		BLocker mLock;
		
		BBitmap *mScreenBitmap;
		
		int32 mAddress;
		char const *mPasswd;
};
