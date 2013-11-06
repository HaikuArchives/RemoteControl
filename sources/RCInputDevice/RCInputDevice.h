#include <InputServerDevice.h>

extern "C" _EXPORT BInputServerDevice* instantiate_input_device();

class RCInputDevice : public BInputServerDevice
{
	public:
		RCInputDevice();
		virtual ~RCInputDevice();
		virtual status_t InitCheck();
		virtual status_t Start(const char *device, void *cookie);
		virtual status_t Stop (const char *device, void *cookie);
		
	private:
		static int32 threadFunc(void *arg);
		bool mActive;
		thread_id mThid;
		status_t mInit;
		
		port_id mPort;
};
