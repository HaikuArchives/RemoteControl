#include <Message.h>

#include "proto.h"
#include "RCInputFilter.h"

BInputServerFilter* instantiate_input_filter()
{
	return (new RCInputFilter());
}

RCInputFilter::RCInputFilter() :
	init(B_OK),
	mDoFilter(false)
{
	mThid=spawn_thread(threadFunc, "RCInputFilter port listener", B_NORMAL_PRIORITY, this);
	if(mThid<0)
	{
		init=mThid;
		return;
	}
	
	mPort=create_port(16, FilterPortName);
	if(mPort<0)
	{
		init=mPort;
		kill_thread(mThid);
		return;
	}
	
	init=resume_thread(mThid);
	if(init!=B_OK)
	{
		kill_thread(mThid);
		delete_port(mPort);
	}
}

RCInputFilter::~RCInputFilter()
{
	if(init==B_OK)
	{
		delete_port(mPort);
		status_t ret;
		wait_for_thread(mThid, &ret);
	}
}

status_t RCInputFilter::InitCheck()
{
	return init;
}

filter_result RCInputFilter::Filter(BMessage *message, BList *outList)
{
	if((message->RemoveData("keep")!=B_OK) && mDoFilter)
		return B_SKIP_MESSAGE;
	else
		return B_DISPATCH_MESSAGE;
}

int32 RCInputFilter::threadFunc(void *cookie)
{
	RCInputFilter *ptr=(RCInputFilter *)cookie;
	int32 code;
	
	while(read_port(ptr->mPort, &code, 0, 0)==B_OK)
	{
		if(code==RCMessage::EnableFilter)
			ptr->mDoFilter=true;
		else
			ptr->mDoFilter=false;
	}
	return B_OK;
}
