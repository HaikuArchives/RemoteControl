#include <Screen.h>

class BBitmap;

class ScreenShot
{
	public:
		ScreenShot();
		~ScreenShot();
		
		void Refresh(bool draw_cursor=true);
		status_t ReadBitmap(BBitmap *buffer, BRect *bounds=0);
		status_t GetBitmap(BBitmap **buffer, BRect *bounds=0);
		
	private:
		BBitmap *mShot;
		BScreen mScreen;
		
		double *ppus;
		int32 cptr;
};
