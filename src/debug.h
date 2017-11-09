#ifndef NODE_SASS_DEBUG_H
#define NODE_SASS_DEBUG_H

#include <sstream>

enum TLogLevel {logINFO, logTRACE};
static TLogLevel LogReportingLevel = getenv("NODESASS_TRACE") ? logTRACE : logINFO;
class Log
{
public:
  Log();
  virtual ~Log();
  std::ostringstream& Get(TLogLevel level, void *p, const char *f, const char *filen, int lineno);
  std::ostringstream& Get(TLogLevel level, const char *f, const char *filen, int lineno);
protected:
  std::ostringstream os;
private:
  Log(const Log&);
  Log& operator =(const Log&);
  TLogLevel messageLevel;
  const char *func;
};

// Visual Studio 2013 does not like __func__
#if _MSC_VER < 1900
#define __func__ __FUNCTION__
#endif

#define TRACE() \
  if (logTRACE > LogReportingLevel) ; \
  else Log().Get(logTRACE, __func__, __FILE__, __LINE__)

#define TRACEINST(obj) \
  if (logTRACE > LogReportingLevel) ; \
  else Log().Get(logTRACE, (obj), __func__, __FILE__, __LINE__)

#endif
