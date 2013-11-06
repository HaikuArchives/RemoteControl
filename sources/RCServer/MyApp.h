#include <Application.h>

class MyApp : public BApplication
{
	public:
		MyApp():BApplication("application/x-vnd.Titoz-RCServer"){}
		
		virtual bool QuitRequested(){return true;}
};
