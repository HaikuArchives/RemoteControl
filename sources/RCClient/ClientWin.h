#include <Window.h>

class ClientView;
class InputClient;
class ScreenClient;

class ClientWin : public BWindow
{
	public:
		ClientWin(BRect frame, InputClient *inputClient, ScreenClient *screenClient);
		~ClientWin();
		virtual bool QuitRequested();
		
	private:
		ClientView *mView;
		InputClient  *mInputClient;
		ScreenClient *mScreenClient;
};
