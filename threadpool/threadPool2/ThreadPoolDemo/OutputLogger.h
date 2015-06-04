/**
 * OutputLogger.h - Declaration of OutputLogger
 */
#ifndef _OUTPUT_LOGGER_H_
#define _OUTPUT_LOGGER_H_

#include "ThreadPool.h"



/**
 * OutputLogger - Log error and information in output window
 */
class OutputLogger : public TP::Logger
{
public:

    OutputLogger();
    ~OutputLogger();
    void LogError( const std::wstring& wstrError_i );
    void LogInfo( const std::wstring& wstrInfo_i );
};

#endif // #ifndef _OUTPUT_LOGGER_H_