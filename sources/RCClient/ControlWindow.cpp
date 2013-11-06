#include <iostream>
#include <stdlib.h>

#include <TextControl.h>
#include <String.h>
#include <StringView.h>
#include <MessageRunner.h>
#include <Button.h>
#include <CheckBox.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <Screen.h>
#include <Application.h>
#include <View.h>

#include "ControlWindow.h"

#include "PixelRateCalculator.h"
#include "ClientWin.h"
#include "InputClient.h"
#include "ScreenClient.h"

ControlWindow::ControlWindow(ClientWin *win, InputClient *inputClient, ScreenClient *screenClient) :
	BWindow(BRect(100, 100, 200, 200), "Control", B_FLOATING_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS),
	mWin(win),
	mInputClient(inputClient),
	mScreenClient(screenClient)
{
	BMessage *pm=mScreenClient->GetPrefs();
	BMessage prefsMsg;
	if(pm)
		prefsMsg=*pm;
	
	mPixelRateCalculator=new PixelRateCalculator(new BMessage(UPDATE_PIXEL_RATE), BMessenger(this));
	mScreenClient->SetPixelRateCalculator(mPixelRateCalculator);
	
	background=new BView(Bounds(), "background", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	background->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(background);
	
	mTranslatorMenu=new BPopUpMenu("Translator");
	
	int32 type=-1;
	prefsMsg.FindInt32("ss_type", &type);
	int32 numTranslators;
	const ScreenClient::TranslatorData *transData=screenClient->GetTranslatorData(&numTranslators);
	for(int32 i=0; i<numTranslators; i++)
	{
		BMessage *msg=new BMessage(TRANSLATOR_MENU);
		msg->AddInt32("type", transData[i].type);
		msg->AddInt32("id", transData[i].id);
		BMenuItem *item=new BMenuItem(transData[i].name, msg);
		if(type==transData[i].type)
			item->SetMarked(true);
		mTranslatorMenu->AddItem(item);
	}
	
	mTranslatorField=new BMenuField(BRect(10, 10, 10, 10), "translator menu", "Translator", mTranslatorMenu);
	mTranslatorField->ResizeToPreferred();
	background->AddChild(mTranslatorField);
	
	mInputBox=new BCheckBox(BRect(10, mTranslatorField->Frame().bottom+5, 10, mTranslatorField->Frame().bottom+5),
							 "mInputBox", "Send input messages", new BMessage(SEND_INPUT_MESSAGES));
	mInputBox->ResizeToPreferred();
	background->AddChild(mInputBox);
	
	mInputFilterBox=new BCheckBox(BRect(10, mInputBox->Frame().bottom+5, 10, mInputBox->Frame().bottom+5),
							 "mInputFilterBox", "Filter remote input", new BMessage(FILTER_REMOTE_INPUT));
	mInputFilterBox->ResizeToPreferred();
	background->AddChild(mInputFilterBox);
	
	mUpdateBox=new BCheckBox(BRect(10, mInputFilterBox->Frame().bottom+5, 10, mInputFilterBox->Frame().bottom+5),
							 "mUpdateBox", "Update active window", new BMessage(UPDATE_ACTIVE));
	mUpdateBox->ResizeToPreferred();
	background->AddChild(mUpdateBox);
	
	mUpdateChangedBox=new BCheckBox(BRect(10, mUpdateBox->Frame().bottom+5, 10, mUpdateBox->Frame().bottom+5),
									"mUpdateChangedBox", "Update changed", new BMessage(UPDATE_CHANGED));
	mUpdateChangedBox->ResizeToPreferred();
	mUpdateChangedBox->SetValue(B_CONTROL_ON);
	background->AddChild(mUpdateChangedBox);
	
	mSendDiffBox=new BCheckBox(BRect(10, mUpdateChangedBox->Frame().bottom+5, 10, mUpdateChangedBox->Frame().bottom+5),
									"mSendDiffBox", "Send difference image", new BMessage(SEND_DIFF));
	mSendDiffBox->ResizeToPreferred();
	mSendDiffBox->SetValue(B_CONTROL_OFF);
	background->AddChild(mSendDiffBox);
	
	mSquareSize=new BTextControl(BRect(10, mSendDiffBox->Frame().bottom+5, 10, mSendDiffBox->Frame().bottom+5),
								 "mSquareSize", "Square size", "100", new BMessage(SQUARE_SIZE));
	mSquareSize->ResizeToPreferred();
	mSquareSize->ResizeBy(-50, 0);
	background->AddChild(mSquareSize);
	
	mCenterButton=new BButton(BRect(10, mSquareSize->Frame().bottom+5, 10, mSquareSize->Frame().bottom+5),
							  "mCenterButton", "Center window", new BMessage(CENTER_WINDOW));
	mCenterButton->ResizeToPreferred();
	background->AddChild(mCenterButton);
	
	mRateView=new BStringView(BRect(10, mCenterButton->Frame().bottom+5, 10, mCenterButton->Frame().bottom+5),
							  "mRateView", "Pixel rate");
	mRateView->ResizeToPreferred();
	background->AddChild(mRateView);
	
	ResizeTo(mUpdateBox->Frame().right+10, mRateView->Frame().bottom+10);
	
	int32 size;
	if(prefsMsg.FindInt32("ss_square_size", &size)==B_OK)
	{
		BString str;
		str <<size;
		mSquareSize->SetText(str.String());
	}
	
	bool update;
	if(prefsMsg.FindBool("ss_update_active", &update)==B_OK)
		mUpdateBox->SetValue(update);
	if(prefsMsg.FindBool("ss_update_changed", &update)==B_OK)
		mUpdateChangedBox->SetValue(update);
	if(prefsMsg.FindBool("ss_send_diff", &update)==B_OK)
		mSendDiffBox->SetValue(update);
}

ControlWindow::~ControlWindow()
{
}

void ControlWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case TRANSLATOR_MENU:
		{
			int32 type;
			int32 id;
			if(msg->FindInt32("type", &type)!=B_OK)
				break;
			if(msg->FindInt32("id", &id)!=B_OK)
				break;
			mScreenClient->ChangeTranslator(type, id);
			break;
		}
		case UPDATE_ACTIVE:
			if(mUpdateBox->Value())
				mScreenClient->UpdateActiveWindow();
			else
				mScreenClient->UpadteAll();
			break;
		case CENTER_WINDOW:
		{
			BRect screenFrame=BScreen().Frame();
			mWin->Lock();
			mWin->MoveTo((screenFrame.right-mWin->Bounds().right)/2, (screenFrame.bottom-mWin->Bounds().bottom)/2);
			mWin->Unlock();
			break;
		}
		case SEND_INPUT_MESSAGES:
			mInputClient->Enable(mInputBox->Value());
			break;
		case FILTER_REMOTE_INPUT:
			mInputClient->EnableFilter(mInputFilterBox->Value());
			break;
		case UPDATE_PIXEL_RATE:
		{
			BString str;
			str <<"Pixel rate: ";
			int32 rate=(int32)mPixelRateCalculator->GetRate();
			if(rate<1000)
				str <<rate;
			else
				if(rate<10000)
					str <<float(rate/10)/float(100.0) <<'K';
				else
					if(rate<1000000)
						str <<rate/1000 <<'K';
					else
						if(rate<1000000000)
							str <<rate/1000000 <<'M';
						else
							str <<rate/1000000000 <<'G';
			mRateView->SetText(str.String());
			mRateView->ResizeToPreferred();
			break;
		}
		case SQUARE_SIZE:
		{
			int32 size=atoi(mSquareSize->Text());
			if(size<1)
				size=100;
			BString str;
			str <<size;
			mSquareSize->SetText(str.String());
			mScreenClient->SetSquareSize(size);
			break;
		}
		case UPDATE_CHANGED:
			if(mUpdateChangedBox->Value())
				mScreenClient->UpdateChanged();
			else
				mScreenClient->UpadteEverything();
			break;
		case SEND_DIFF:
			mScreenClient->SendDiff(mSendDiffBox->Value());
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}

bool ControlWindow::QuitRequested()
{
	mScreenClient->Stop();
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	return true;
}
