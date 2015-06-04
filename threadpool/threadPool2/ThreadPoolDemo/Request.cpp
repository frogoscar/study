/**
 * Request.cpp - Implementation of request class.
 */

#include "Request.h"

/** 
 * Constructor
 */
Request::Request()
{
}


/** 
 * Destructor
 */
Request::~Request()
{
}


/** 
 * Override thread procedure.
 */
long Request::Execute()
{
    std::wstring wstrLog( L"Execution started" );
    PrepareLog( wstrLog );
    OutputDebugString( wstrLog.c_str());
    Sleep( 7000 );
    if( IsAborted())
    {
        wstrLog = L"Request aborted";
        PrepareLog( wstrLog );
        OutputDebugString( wstrLog.c_str());
        return 0;
    }
    Sleep( 1000 );
    wstrLog = L"Execution completed";
    PrepareLog( wstrLog );
    OutputDebugString( wstrLog.c_str());
    return 0;
}