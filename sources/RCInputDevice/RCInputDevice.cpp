#include <string.h>

#include "dbout.h"
#include "proto.h"
#include "RCInputDevice.h"

BInputServerDevice* instantiate_input_device()
{
	return (new RCInputDevice());
}

RCInputDevice::RCInputDevice() :
	mInit(B_OK)
{
	input_device_ref nervousDevice = {"Remote Device", B_POINTING_DEVICE, NULL};
	input_device_ref *nervousDeviceList[2] = {&nervousDevice, NULL};
	
	mInit=RegisterDevices(nervousDeviceList);
	if(mInit!=B_OK)
	{
		dbout <<"couldn't register device -- " <<strerror(mInit) <<endl;
		return;
	}
	
	mThid=spawn_thread(threadFunc, "Remote Device", B_LOW_PRIORITY, this);
	if(mThid<0)
	{
		mInit=mThid;
		dbout <<"couldn't spawn thread -- " <<strerror(mInit) <<endl;
		return;
	}
	
	mInit=resume_thread(mThid);
	if(mInit<0)
	{
		dbout <<"couldn't resume thread -- " <<strerror(mInit) <<endl;
		return;
	}
	
	mPort=create_port(16, PortName);
	if(mPort<0)
	{
		mInit=mPort;
		dbout <<"couldn't create port -- " <<strerror(mInit) <<endl;
		kill_thread(mThid);
		return;
	}
	// dbout <<"constructeur ok" <<endl;
}

RCInputDevice::~RCInputDevice()
{
	// dbout <<"destructeur" <<endl;
	
	status_t err = B_OK;
	
	if(mInit==B_OK)
	{
		delete_port(mPort);
		wait_for_thread(mThid, &err);
	}
}

status_t RCInputDevice::InitCheck()
{
	dbout <<"mInit==" <<strerror(mInit) <<endl;
	
	return mInit;
}

status_t RCInputDevice::Start(const char *device, void *cookie)
{
	dbout <<"start" <<endl;
	mActive=true;
	return B_OK;
}

status_t RCInputDevice::Stop(const char *device, void *cookie)
{
	// dbout <<"stop" <<endl;
	mActive = false;
	return B_OK;
}

int32 RCInputDevice::threadFunc(void *arg)
{
	RCInputDevice *mDevice = (RCInputDevice *)arg;
	
	// dbout <<"threadFunc" <<endl;
	
	while(1)
	{
		int32 code;
		int32 size;
		size=port_buffer_size(mDevice->mPort);
		if(size<0)
			return size;
		char *flat=new char[size];
		size=read_port(mDevice->mPort, &code, flat, size);
		if(size<0)
			return size;
		
		BMessage *msg=new BMessage();
		msg->Unflatten(flat);
		
		bigtime_t now=system_time();
		msg->ReplaceInt64("when", now);
		msg->AddInt32("keep", 0);
		
		mDevice->EnqueueMessage(msg);
		delete flat;
	}
	
	return (B_OK);
}
