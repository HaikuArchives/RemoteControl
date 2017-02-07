#include <fstream>

#ifdef DBOUT_TO_STDOUT
extern std::ostream &dbout;
#else
extern std::ofstream dbout;
#endif
