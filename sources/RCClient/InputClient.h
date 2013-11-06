class ClientIO;
class BMessage;

class InputClient
{
	public:
		InputClient(ClientIO *clio, char const *passwd);
		~InputClient();
		
		status_t InitCheck();
		
		void Enable(bool enable=true){mEnabled=enable;}
		void SendMessage(const BMessage *msg);
		void EnableFilter(bool enable=true);
		
	private:
		ClientIO *mClio;
		bool mEnabled;
		status_t mInit;
};
