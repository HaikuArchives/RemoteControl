class ClientIO;

class Password
{
	public:
		Password(ClientIO *clio);
		~Password();
		
		bool SendPassword(char const *passwd);
		bool RecvPassword(char const *passwd);
		
	private:
		Password();
		
		ClientIO *mClio;
};
