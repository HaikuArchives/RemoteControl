#include <View.h>

class BBitmap;

class InputClient;
class ScreenClient;

class ClientView : public BView
{
	public:
		ClientView(BRect frame, InputClient *inputClient, ScreenClient *screenClient);
		~ClientView();
		
		virtual void MessageReceived(BMessage *msg);
		virtual void MouseDown(BPoint point);
		virtual void MouseUp(BPoint point);
		virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
		virtual void KeyDown(const char *bytes, int32 numBytes);
		virtual void KeyUp  (const char *bytes, int32 numBytes);
		virtual void Draw(BRect updateRect);
		
		virtual void AttachedToWindow();
		
		void CopyBitmap(const BBitmap *bmp, BRect frame, bool diff);
		
	private:
		void AddBitmap(BBitmap *bmp1, BRect frame, const BBitmap *bmp2);
		void CopyBitmap(BBitmap *bmp1, BRect frame, const BBitmap *bmp2);
		void SendMessage(BMessage *msg);
		
		static const uint32 UPDATE='updt';
		
		InputClient  *mInputClient;
		ScreenClient *mScreenClient;
		
		BBitmap *mBmp;
};
