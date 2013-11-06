#include <Window.h>

class BPopUpMenu;
class BMenuField;
class BCheckBox;
class BButton;
class BStringView;
class BTextControl;

class PixelRateCalculator;
class InputClient;
class ScreenClient;
class ClientWin;

class ControlWindow : public BWindow
{
	public:
		ControlWindow(ClientWin *win, InputClient *inputClient, ScreenClient *screenClient);
		~ControlWindow();
		
		virtual void MessageReceived(BMessage *msg);
		virtual bool QuitRequested();
		
	private:
		static const uint32 TRANSLATOR_MENU    ='tram';
		static const uint32 UPDATE_ACTIVE      ='uact';
		static const uint32 CENTER_WINDOW      ='cwin';
		static const uint32 SEND_INPUT_MESSAGES='sinp';
		static const uint32 FILTER_REMOTE_INPUT='frin';
		static const uint32 UPDATE_PIXEL_RATE  ='urat';
		static const uint32 SQUARE_SIZE        ='ssiz';
		static const uint32 UPDATE_CHANGED     ='uchg';
		static const uint32 SEND_DIFF          ='sdif';
		
		ClientWin *mWin;
		InputClient  *mInputClient;
		ScreenClient *mScreenClient;
		
		BView *background;
		BPopUpMenu *mTranslatorMenu;
		BMenuField *mTranslatorField;
		
		BCheckBox *mInputBox;
		BCheckBox *mInputFilterBox;
		BCheckBox *mUpdateBox;
		BCheckBox *mUpdateChangedBox;
		BCheckBox *mSendDiffBox;
		BTextControl *mSquareSize;
		BButton *mCenterButton;
		
		PixelRateCalculator *mPixelRateCalculator;
		BStringView *mRateView;
};
