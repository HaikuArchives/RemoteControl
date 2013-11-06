#include <Message.h>
#include <Locker.h>

class Preferences : public BLocker
{
	public:
		static Preferences *Instance(){return &mInstance;}
		
		void SetFile(const char *filename);
		status_t Save(const BMessage *msg, int32 key);
		status_t Load(      BMessage *msg, int32 key);
		
	private:
		Preferences();
		~Preferences();
		
		static Preferences mInstance;
		
		int32 GetKeyIndx(int32 key);
		
		char *mFilename;
		
		BMessage mPrefs;
};
