#include <File.h>
#include <Autolock.h>

#include "Preferences.h"

Preferences Preferences::mInstance;

Preferences::Preferences() :
	mFilename(0)
{
}

Preferences::~Preferences()
{
	if(mFilename)
		delete mFilename;
}

void Preferences::SetFile(const char *filename)
{
	if(mFilename)
		delete mFilename;
	if(!filename)
	{
		mFilename=0;
		return;
	}
	
	mFilename=new char[strlen(filename)+1];
	strcpy(mFilename, filename);
}

status_t Preferences::Save(const BMessage *msg, int32 key)
{
	if(!mFilename)
		return B_ERROR;
	
	BAutolock autolock(this);
	if(!autolock.IsLocked())
		return B_ERROR;
	
	status_t err;
	
	int32 indx=GetKeyIndx(key);
	if(indx<0)
	{
		err=mPrefs.AddInt32("indices", key);
		if(err!=B_OK)
			return err;
		err=mPrefs.AddMessage("messages", msg);
		if(err!=B_OK)
		{
			type_code code;
			int32 count;
			mPrefs.GetInfo("indices", &code, &count);
			mPrefs.RemoveData("indices", count-1);
			return err;
		}
		
	}
	else
	{
		err=mPrefs.ReplaceMessage("messages", indx, msg);
		if(err!=B_OK)
			return err;
	}
	
	BFile file(mFilename, B_CREATE_FILE | B_WRITE_ONLY);
	err=file.InitCheck();
	if(err!=B_OK)
		return err;
	return mPrefs.Flatten(&file);
}

status_t Preferences::Load(BMessage *msg, int32 key)
{
	if(!mFilename)
		return B_ERROR;
	
	BAutolock autolock(this);
	if(!autolock.IsLocked())
		return B_ERROR;
	
	BFile file(mFilename, B_READ_ONLY);
	status_t err=file.InitCheck();
	if(err!=B_OK)
		return err;
	err=mPrefs.Unflatten(&file);
	if(err!=B_OK)
		return err;
	
	int32 indx=GetKeyIndx(key);
	if(indx<0)
		return B_ERROR;
	return mPrefs.FindMessage("messages", indx, msg);
}

int32 Preferences::GetKeyIndx(int32 key)
{
	int32 i=0;
	int32 k;
	
	while(mPrefs.FindInt32("indices", i, &k)==B_OK)
		if(k==key)
			return i;
		else
			i++;
	return -1;
}
