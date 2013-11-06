extern const char *PortName;
extern const char *FilterPortName;

static const int MsgCode='msg ';

struct RCMessage
{
	static const int ChangeTranslator  ='chtr';
	static const int UpdateActiveWindow='uawd';
	static const int UpadteAll         ='uall';
	static const int UpadteChanged     ='uchg';
	static const int UpadteEverything  ='ueve';
	static const int FlatMessage       ='flat';
	static const int EnableFilter      ='efil';
	static const int DisableFilter     ='dfil';
	static const int BitmapFollowing   ='bmpf';
	static const int CouldntTranslate  ='ctrn';
	static const int CouldntGetBitmap  ='cgtb';
	static const int SetSquareSize     ='srsz';
	static const int SendDiff          ='sdif';
	static const int RequirePassword   ='rqpw';
	static const int NoPassword        ='nopw';
	static const int PasswordOK        ='pwok';
	static const int BadPassword       ='bdpw';
};
