#include <OS.h>

class ClientIO;

class InputServer
{
	public:
		InputServer(ClientIO *clio, char const *passwd);
		~InputServer();
		
	private:
		static int32 threadFunc(void *cookie);
		int32 ThreadFunc();
		
		ClientIO *mClio;
		thread_id mThid;
		port_id mPort;
		port_id mFilterPort;
		
		char const *mPasswd;
};
