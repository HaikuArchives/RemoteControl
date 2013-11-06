#include <OS.h>

class ScreenListener
{
	public:
		ScreenListener(int32 sock, char const *passwd);
		~ScreenListener();
		
	private:
		static int32 threadFunc(void *cookie);
		int32 ThreadFunc();
		
		int32 mSock;
		thread_id mThid;
		
		char const *mPasswd;
};
