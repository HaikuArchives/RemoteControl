#include "dbout.h"

#ifdef DBOUT_TO_STDOUT
std::ostream &dbout=cout;
#else
std::ofstream dbout("/boot/home/debug/RCInputDevice", std::ios::app);
#endif
