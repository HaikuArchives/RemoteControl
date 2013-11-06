#include <fstream.h>

#ifdef DBOUT_TO_STDOUT
extern ostream &dbout;
#else
extern ofstream dbout;
#endif
