#include <Application.h>

class InputClient;
class ScreenClient;
class ClientWin;
class ControlWindow;

class ClientApp : public BApplication
{
	public:
		ClientApp();
		~ClientApp();
		
		virtual void ArgvReceived(int32 argc, char **argv);
		virtual void ReadyToRun();
		virtual bool QuitRequested();
		
	private:
		void usage(char *name);
		status_t parse(int argc, char **argv);
		int32 Connect(int32 port, const char *server);
		
		InputClient  *mInputClient;
		ScreenClient *mScreenClient;
		
		ClientWin *mWin;
		ControlWindow *mControlWin;
		bool mArgsReceived;
		
		int32 mPorti;
		int32 mPorts;
		char *mHostname;
		char const *mPasswd;
};
