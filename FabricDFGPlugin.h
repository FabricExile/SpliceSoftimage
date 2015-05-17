#ifndef __FabricDFGPlugin_H_ 
#define __FabricDFGPlugin_H_

#define FabricDFGPlugin_BETA  1   // if != 0 then this is a beta.

// disable some annoying VS warnings.
#pragma warning(disable : 4530)   // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc.
#pragma warning(disable : 4800)   // forcing value to bool 'true' or 'false'.
#pragma warning(disable : 4806)   // unsafe operation: no value of type 'bool' promoted to type ...etc.

// includes.
#include <string>

// forward declaration: log system.
void feLog(void *userData, const char *s, unsigned int length);
void feLog(void *userData, const std::string &s);
void feLog(const std::string &s);
void feLogError(void *userData, const char *s, unsigned int length);
void feLogError(void *userData, const std::string &s);
void feLogError(const std::string &s);

#endif
