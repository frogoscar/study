/**
 * Request.h - Declaration of Request class
 */

#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "ThreadPool.h"

/**
 * Request : Handle  a single request.
 */
class Request : public TP::AbstractRequest
{
public:
    Request();
    ~Request();
    long Execute();
};
#endif // #ifndef _REQUEST_H_