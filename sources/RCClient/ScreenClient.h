#include <Rect.h>
#include <OS.h>

class ClientView;
class ClientIO;
class PixelRateCalculator;

class ScreenClient
{
	public:
		ScreenClient(ClientIO *clio, char const *passwd);
		~ScreenClient();
		
		status_t InitCheck();
		
		struct TranslatorData
		{
			int32 type;
			int32 id;
			char *name;
		};
		
		void Stop();
		BRect GetScreenFrame(){return mScreenFrame;}
		BMessage *GetPrefs(){return mPrefs;}
		const TranslatorData *GetTranslatorData(int32 *numTrans){*numTrans=mNumTrans; return mTransData;}
		void ChangeTranslator(int32 type, int32 id);
		void UpdateActiveWindow();
		void UpadteAll();
		void UpdateChanged();
		void UpadteEverything();
		void SetSquareSize(int32 size);
		void SendDiff(bool sendDiff);
		
		void SetView(ClientView *view);
		void SetPixelRateCalculator(PixelRateCalculator *calc);
		
	private:
		BRect ReceiveFrame();
		void ReceiveTranslators();
		void ReceivePrefs();
		BBitmap *ReceiveBitmap();
		
		static int32 threadFunc(void *cookie);
		int32 ThreadFunc();
		
		ClientIO *mClio;
		thread_id mThid;
		port_id mPort;
		
		ClientView *mView;
		
		BRect mScreenFrame;
		
		bool mQuit;
		
		int32 mNumTrans;
		TranslatorData *mTransData;
		PixelRateCalculator *mPixelRateCalculator;
		
		BMessage *mPrefs;
		
		status_t mInit;
};
