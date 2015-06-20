#include <stdio.h>
#include <sstream>

#include <uv.h>

#include "debug.h"

Log::Log() {} 

std::ostringstream& Log::Get(TLogLevel level, void *p, const char *f, const char *filen, int lineno)
{
   os << "[NODESASS@" << uv_thread_self() << "] " << p << ":" << f << " " << filen << ":" << lineno << " ";
   messageLevel = level;
   return os;
}
std::ostringstream& Log::Get(TLogLevel level, const char *f, const char *filen, int lineno)
{
   os << "[NODESASS@" << uv_thread_self() << "] " << f << " " << filen << ":" << lineno << " ";
   messageLevel = level;
   return os;
}
Log::~Log()
{
   os << std::endl;
   fprintf(stderr, "%s", os.str().c_str());
   fflush(stderr);
}
