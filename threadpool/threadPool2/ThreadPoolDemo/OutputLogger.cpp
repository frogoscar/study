/**
 * OutputLogger.cpp - Implementation of OutputLogger
 */
#include <windows.h>
#include "OutputLogger.h"


/** 
 * Constructor
 */
OutputLogger::OutputLogger()
{
}


/** 
 * Destructor
 */
OutputLogger::~OutputLogger()
{
}


/** 
 * Log error
 * 
 * @param       wstrError_i - Error description
 */
void OutputLogger::LogError( const std::wstring& wstrError_i )
{
    OutputDebugString( wstrError_i.c_str());
}


/** 
 * Log information
 * 
 * @param       wstrInfo_i - Information log
 */
void OutputLogger::LogInfo( const std::wstring& wstrInfo_i )
{
    OutputDebugString( wstrInfo_i.c_str());
}