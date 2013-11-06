#include <OS.h>

class InputListener
{
	public:
		InputListener(int32 sock, char const *passwd);
		~InputListener();
		
	private:
		static int32 threadFunc(void *cookie);
		int32 ThreadFunc();
		
		int32 mSock;
		thread_id mThid;
		
		char const *mPasswd;
};
