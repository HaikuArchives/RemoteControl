#include "dbout.h"

#ifdef DBOUT_TO_STDOUT
ostream &dbout=cout;
#else
ofstream dbout("/boot/home/debug/RCInputDevice", ios::app);
#endif
