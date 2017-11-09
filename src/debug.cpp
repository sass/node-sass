#include <stdio.h>
#include <sstream>

#include <uv.h>

#include "debug.h"

Log::Log() {} 

std::ostringstream& Log::Get(TLogLevel level, void *p, const char *f, const char *filen, int lineno)
{
   os << "[NODESASS@" << uv_thread_self() << "] " << p << ":" << filen << ":" << lineno << " ";
   func = f;
   messageLevel = level;
   return os;
}
std::ostringstream& Log::Get(TLogLevel level, const char *f, const char *filen, int lineno)
{
   os << "[NODESASS@" << uv_thread_self() << "] " << filen << ":" << lineno << " ";
   func = f;
   messageLevel = level;
   return os;
}
Log::~Log()
{
   os << " (" << func << ")" << std::endl;
   fprintf(stderr, "%s", os.str().c_str());
   fflush(stderr);
}
