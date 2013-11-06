#ifndef CLIENTIO_H
#define CLIENTIO_H

#include <SupportDefs.h>

class ClientDisconnected{};

class ClientIO
{
	public:
		ClientIO(int32 sock);
		~ClientIO();
		
		void Flush();
		void ReadBytes(void *buf, int32 size);
		void WriteBytes(const void *buf, int32 size);
		int32 CloseSocket();
		
	private:
		void Fill();
		
		int32 mSock;
		
		static const int32 BUFFSIZE=16384;
		
		int8 mInBuffer[BUFFSIZE];
		int32 mInPos;
		int32 mInSize;
		
		int8 mOutBuffer[BUFFSIZE];
		int32 mOutPos;
};

#endif
