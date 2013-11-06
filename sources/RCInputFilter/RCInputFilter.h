#include <InputServerFilter.h>

extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

class RCInputFilter : public BInputServerFilter
{
	public:
		RCInputFilter();
		virtual ~RCInputFilter();
		virtual filter_result Filter(BMessage *message, BList *outList);
		virtual status_t InitCheck();
		
	private:
		static int32 threadFunc(void *cookie);
		
		status_t init;
		bool mDoFilter;
		
		thread_id mThid;
		port_id mPort;
};
