#include <iostream>
#include <string.h>

#include <Window.h>
#include <Message.h>
#include <Bitmap.h>

#include "InputClient.h"
#include "ScreenClient.h"

#include "ClientView.h"

ClientView::ClientView(BRect frame, InputClient *inputClient, ScreenClient *screenClient) :
	BView(frame, "ClientView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	mInputClient(inputClient),
	mScreenClient(screenClient)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	if(mScreenClient)
		mScreenClient->SetView(this);
	mBmp=new BBitmap(Bounds(), B_RGB32);
	memset(mBmp->Bits(), 0, mBmp->BitsLength());
}

ClientView::~ClientView()
{
	delete mBmp;
}

void ClientView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case UPDATE:
		{
			BRect rect;
			if(msg->FindRect("rect", &rect)!=B_OK)
				break;
			DrawBitmap(mBmp, rect, rect);
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}

void ClientView::MouseDown(BPoint point)
{
	SendMessage(Window()->CurrentMessage());
}

void ClientView::MouseUp(BPoint point)
{
	SendMessage(Window()->CurrentMessage());
}

void ClientView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	SendMessage(Window()->CurrentMessage());
}

void ClientView::KeyDown(const char *bytes, int32 numBytes)
{
	SendMessage(Window()->CurrentMessage());
}

void ClientView::KeyUp(const char *bytes, int32 numBytes)
{
	SendMessage(Window()->CurrentMessage());
}

void ClientView::AttachedToWindow()
{
	MakeFocus();
}

void ClientView::SendMessage(BMessage *msg)
{
	if(mInputClient)
	{
		int32 x, y;
		if(msg->FindInt32("x", &x)==B_OK)
		{
			msg->RemoveData("x");
			msg->AddFloat("x", float(x)/Bounds().Width());
		}
		if(msg->FindInt32("y", &y)==B_OK)
		{
			msg->RemoveData("y");
			msg->AddFloat("y", float(y)/Bounds().Width());
		}
		
		mInputClient->SendMessage(msg);
	}
}

void ClientView::Draw(BRect updateRect)
{
	DrawBitmap(mBmp, updateRect, updateRect);
}

void ClientView::AddBitmap(BBitmap *bmp1, BRect frame, const BBitmap *bmp2)
{
	char *bits1=(char *)bmp1->Bits();
	const char *bits2=(char *)bmp2->Bits();
	int32 bpr1=bmp1->BytesPerRow();
	int32 bpr2=bmp2->BytesPerRow();
	int32 height=int32(frame.Height()+1);
	int32 offset=int32(frame.left)*4+int32(frame.top)*bpr1;
	for(int32 i=0; i<height; i++)
		for(int32 j=0; j<bpr2; j++)
			bits1[offset+i*bpr1+j]+=bits2[i*bpr2+j];
	
//	for(int32 i=0; i<height; i++)
//	{
//		bits1[offset+i*bpr1+2]=255;
//		bits1[offset+i*bpr1+bpr2-2]=255;
//	}
//	for(int32 j=2; j<bpr2-2; j+=4)
//	{
//		bits1[offset+j]=255;
//		bits1[offset+(height-1)*bpr1+j]=255;
//	}
}

void ClientView::CopyBitmap(BBitmap *bmp1, BRect frame, const BBitmap *bmp2)
{
	char *bits1=(char *)bmp1->Bits();
	const char *bits2=(char *)bmp2->Bits();
	int32 bpr1=bmp1->BytesPerRow();
	int32 bpr2=bmp2->BytesPerRow();
	int32 height=int32(frame.Height()+1);
	int32 offset=int32(frame.left)*4+int32(frame.top)*bpr1;
	for(int32 i=0; i<height; i++)
		memcpy(&(bits1[offset+i*bpr1]), &(bits2[i*bpr2]), bpr2);
	
//	for(int32 i=0; i<height; i++)
//	{
//		bits1[offset+i*bpr1+2]=255;
//		bits1[offset+i*bpr1+bpr2-2]=255;
//	}
//	for(int32 j=2; j<bpr2-2; j+=4)
//	{
//		bits1[offset+j]=255;
//		bits1[offset+(height-1)*bpr1+j]=255;
//	}
}

void ClientView::CopyBitmap(const BBitmap *bmp, BRect frame, bool diff)
{
	if(diff)
		AddBitmap(mBmp, frame, bmp);
	else
		CopyBitmap(mBmp, frame, bmp);
	
	BMessage msg(UPDATE);
	msg.AddRect("rect", frame);
	Window()->PostMessage(&msg, this);
}
