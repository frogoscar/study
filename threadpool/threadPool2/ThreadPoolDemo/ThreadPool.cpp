/**
 * @author :    Suresh
 */

#include "ThreadPool.h"
#include <sstream>
#include <iomanip>

namespace TP
{

    /** 
     * Log error description.
     * 
     * @param       lActiveReq_i - Count of active requests.
     * @param       wstrError_i  - Error message.
     */
    void Logger::LogError( const long lActiveReq_i, const std::wstring& wstrError_i )
    {
        std::wstring wstrLog( wstrError_i );
        PrepareLog( lActiveReq_i, wstrLog );
        LogError( wstrLog );
    }


    /** 
     * Log information.
     * 
     * @param       lActiveReq_i - Count of active requests.
     * @param       wstrInfo_i   - Information message.
     */
    void Logger::LogInfo( const long lActiveReq_i, const std::wstring& wstrInfo_i )
    {
        std::wstring wstrLog( wstrInfo_i );
        PrepareLog( lActiveReq_i, wstrLog );
        LogInfo( wstrLog );
    }


    /** 
     * Override this function to log errors. Default log will be in output window.
     * 
     * @param       wstrError_i  - Error description
     */
    void Logger::LogError( const std::wstring& wstrError_i )
    {
        OutputDebugString( wstrError_i.c_str());
    }


    /** 
     * Override this function to log informations. Default log will be in output window.
     * 
     * @param       wstrInfo_i   - Information description.
     */
    void Logger::LogInfo( const std::wstring& wstrInfo_i )
    {
        OutputDebugString( wstrInfo_i.c_str());
    }


    /** 
     * Log thread ID, Active thread count and last error.
     * 
     * @param       lActiveReq_i - Active thread count.
     * @param       wstrLog_io   - Error or information description
     */
    void Logger::PrepareLog( const long lActiveReq_i, std::wstring& wstrLog_io )
    {
        std::wstringstream wstrmLog;
        wstrmLog << L"##TP## [TID=" << std::setfill( L'0' ) << std::setw(8) << ::GetCurrentThreadId()
                 << L"] [ACTIVE REQUEST=" << std::setw(4) << lActiveReq_i
                 << L"] [LAST ERROR=" << std::setw(4) << ::GetLastError()
                 << L"] " << wstrLog_io.c_str() << + L"]";
        wstrLog_io = wstrmLog.str();
    }


    /** 
     * Prepare error or information log.
     * 
     * @param       wstrLog_io - Log information
     */
    void AbstractRequest::PrepareLog( std::wstring& wstrLog_io )
    {
        std::wstringstream wstrmLog;
        wstrmLog << std::setfill( L'0' );
        wstrmLog << L"##RQ## [RID=" << std::setw(8) << GetRequestID()
                 << L"] [Desc=" << wstrLog_io.c_str() << + L"]";
        wstrLog_io = wstrmLog.str();
    }


    /** 
     * Constructor
     */
    ThreadPool::ThreadPool() : m_bDestroyed( false ),
                               m_usThreadCount( 0u ),
                               m_usSemaphoreCount( 0u ),
                               m_lActiveThread( 0u ),
                               m_usPendingReqCount( 0u ),
                               m_hSemaphore( NULL ),
                               m_phThreadList( NULL ),
                               m_pLogger( &m_Logger )
    {
    }


    /** 
     * Destructor
     */
    ThreadPool::~ThreadPool()
    {
        if( NULL != m_phThreadList )
        {
            if( !Destroy())
            {
                LogError( L"Destroy() failed" );
            }
        }
    }


    /** 
     * Create thread pool with specified number of threads.
     * 
     * @param       usThreadCount_i - Thread count.
     * @param       pLogger_i       - Logger instance to log errors and informations
     */
    bool ThreadPool::Create( const unsigned short usThreadCount_i, Logger* pLogger_i )
    {
        try
        {
            // Assign logger object. If user not provided then use existing and
            // error will be logged in output window.
            m_pLogger = ( NULL != pLogger_i ) ? pLogger_i : &m_Logger;
            // Check thread pool is initialized already.
            if( NULL != m_phThreadList )
            {
                LogError( L"ThreadPool already created" );
                return false;
            }
            // Validate thread count.
            if( 0 == usThreadCount_i )
            {
                LogError( L"Minimum allowed thread count is one" );
                return false;
            }
            if( usThreadCount_i > 64 )
            {
                LogError( L"Maximum allowed thread count is 64" );
                return false;
            }
            LogInfo( L"Thread pool creation requested" );

            // Initialize values.
            m_lActiveThread = 0u;
            m_usSemaphoreCount = 0u;
            m_usPendingReqCount = 0u;
            m_usThreadCount = usThreadCount_i;
            // Create semaphore for thread count management.
            m_hSemaphore = CreateSemaphore( NULL, 0, m_usThreadCount, NULL );
            if( NULL == m_hSemaphore )
            {
                LogError( L"Semaphore creation failed" );
                m_usThreadCount = 0u;
                return false;
            }
            // Create worker threads and make pool active
            if( !AddThreads())
            {
                LogError( L"Threads creation failed" );
                Destroy();
                return false;
            }
            SetDestroyFlag( false );
            LogInfo( L"Thread pool created successfully" );
            return true;
        }
        catch( ... )
        {
            LogError( L"Exception occurred in Create()" );
            return false;
        }
    }


    /** 
     * Destroy thread pool.
     */
    bool ThreadPool::Destroy()
    {
        try
        {
            // Check whether thread pool already destroyed.
            if( NULL == m_phThreadList )
            {
                LogError( L"ThreadPool is already destroyed or not created yet" );
                return false;
            }
            // Cancel all requests.
            CancelRequests();
            // Set destroyed flag to true for exiting threads.
            SetDestroyFlag( true );
            // Release remaining semaphores to exit thread.
            {
                AutoLock LockThread( m_LockWorkerThread );
                if( m_lActiveThread < m_usThreadCount )
                {
                    if( NULL == ReleaseSemaphore( m_hSemaphore, m_usThreadCount - m_lActiveThread, NULL ))
                    {
                        LogError( L"Failed to release Semaphore" );
                        return false;
                    }
                }
            }
            // Wait for destroy completion and clean the thread pool.
            if( !DestroyPool())
            {
                LogError( L"Thread pool destruction failed" );
                return false;
            }
            LogInfo( L"Thread Pool destroyed successfully" );
            return true;
        }
        catch( ... )
        {
            LogError( L"Exception occurred in Destroy()" );
            return false;
        }
    }


    /** 
     * Post request to thread pool for processing
     * 
     * @param       pRequest_io - Request to be processed.
     */
    bool ThreadPool::PostRequest( AbstractRequest* pRequest_io )
    {
        try
        {
            AutoLock LockThread( m_LockWorkerThread );
            if( NULL == m_phThreadList )
            {
                LogError( L"ThreadPool is destroyed or not created yet" );
                return false;
            }
            m_RequestQueue.push_back( pRequest_io );
            if( m_usSemaphoreCount < m_usThreadCount )
            {
                // Thread available to process, so notify thread.
                if( !NotifyThread())
                {
                    LogError( L"NotifyThread failed" );
                    // Request notification failed. Try after some time.
                    m_usPendingReqCount++;
                    return false;
                }
            }
            else
            {
                // Thread not available to process.
                m_usPendingReqCount++;
            }
            return true;
        }
        catch( ... )
        {
            LogError( L"Exception occurred in PostRequest()" );
            return false;
        }
    }


    /** 
     * Pop request from queue for processing.
     * 
     * @param       RequestQueue_io  - Request queue.
     * @return      AbstractRequest* - Request pointer.
     */
    AbstractRequest* ThreadPool::PopRequest( REQUEST_QUEUE& RequestQueue_io )
    {
        AutoLock LockThread( m_LockWorkerThread );
        if( !RequestQueue_io.empty())
        {
            AbstractRequest* pRequest = RequestQueue_io.front();
            RequestQueue_io.remove( pRequest );
            return pRequest;
        }
        return 0;
    }


    /** 
     * Create specified number of threads. Initial status of threads will be waiting.
     */
    bool ThreadPool::AddThreads()
    {
        try
        {
            // Allocate memory for all threads.
            m_phThreadList = new HANDLE[m_usThreadCount];
            if( NULL == m_phThreadList )
            {
                LogError( L"Memory allocation for thread handle failed" );
                return false;
            }
            // Create worker threads.
            DWORD dwThreadID = 0;
            for( unsigned short usIdx = 0u; usIdx < m_usThreadCount; usIdx++ )
            {
                // Create worker thread
                m_phThreadList[usIdx] = CreateThread( 0, 0,
                                                      reinterpret_cast<LPTHREAD_START_ROUTINE>( ThreadPool::ThreadProc ),
                                                      this, 0, &dwThreadID );
                if( NULL == m_phThreadList[usIdx] )
                {
                    LogError( L"CreateThread failed" );
                    return false;
                }
            }
            return true;
        }
        catch( ... )
        {
            LogError( L"Exception occurred in AddThreads()" );
            return false;
        }
    }


    /** 
     * Add request to queue and release semaphore by one.
     */
    bool ThreadPool::NotifyThread()
    {
        try
        {
            AutoLock LockThread( m_LockWorkerThread );
            // Release semaphore by one to process this request.
            if( NULL == ReleaseSemaphore( m_hSemaphore, 1, NULL ))
            {
                LogError( L"ReleaseSemaphore failed" );
                return false;
            }
            m_usSemaphoreCount++;
            return true;
        }
        catch( ... )
        {
            LogError( L"Exception occurred in NotifyThread()" );
            m_RequestQueue.pop_back();
            return false;
        }
    }


    /** 
     * Process request in queue.
     */
    bool ThreadPool::ProcessRequests()
    {
        bool bContinue( true );
        do
        {
            try
            {
                LogInfo( L"Thread WAITING" );
                // Wait for request.
                if( !WaitForRequest())
                {
                    LogError( L"WaitForRequest() failed" );
                    continue;
                }
                // Thread counter.
                AutoCounter Counter( m_lActiveThread, m_LockWorkerThread );
                LogInfo( L"Thread ACTIVE" );
                // Check thread pool destroy request.
                if( IsDestroyed())
                {
                    LogInfo( L"Thread EXITING" );
                    break;
                }
                // Get request from request queue.
                AbstractRequest* pRequest = PopRequest( m_RequestQueue );
                if( NULL == pRequest )
                {
                    LogError( L"PopRequest failed" );
                    continue;
                }
                // Execute the request.
                long lReturn = pRequest->Execute();
                if( NULL != lReturn )
                {
                    LogError( L"Request execution failed" );
                    continue;
                }
                // Check thread pool destroy request.
                if( IsDestroyed())
                {
                    LogInfo( L"Thread EXITING" );
                    break;
                }
                AutoLock LockThread( m_LockWorkerThread );
                // Inform thread if any pending request.
                if( m_usPendingReqCount > 0 )
                {
                    if( m_usSemaphoreCount < m_usThreadCount )
                    {
                        // Thread available to process, so notify thread.
                        if( !NotifyThread())
                        {
                            LogError( L"NotifyThread failed" );
                            continue;
                        }
                        m_usPendingReqCount--;
                    }
                }
            }
            catch( ... )
            {
                LogError( L"Exception occurred in ProcessRequests()" );
                continue;
            }
        }
        while( bContinue );
        return true;
    }


    /** 
     * Wait for request queuing to thread pool.
     */
    bool ThreadPool::WaitForRequest()
    {
        try
        {
            // Wait released when requested queued.
            DWORD dwReturn = WaitForSingleObject( m_hSemaphore, INFINITE );
            if( WAIT_OBJECT_0 != dwReturn )
            {
                LogError( L"WaitForSingleObject failed" );
                return false;
            }
            AutoLock LockThread( m_LockWorkerThread );
            m_usSemaphoreCount--;
            // Clear previous error.
            ::SetLastError( 0 );
            return true;
        }
        catch( ... )
        {
            LogError( L"Exception occurred in WaitForRequest()" );
            return false;
        }
    }


    /** 
     * Destroy and clean up thread pool.
     */
    bool ThreadPool::DestroyPool()
    {
        try
        {
            // Wait for the exist of threads.
            DWORD dwReturn = WaitForMultipleObjects( m_usThreadCount, m_phThreadList, TRUE, INFINITE );
            if( WAIT_OBJECT_0 != dwReturn )
            {
                LogError( L"WaitForMultipleObjects failed" );
                return false;
            }
            // Close all threads.
            for( USHORT uIdx = 0u; uIdx < m_usThreadCount; uIdx++ )
            {
                if( TRUE != CloseHandle( m_phThreadList[uIdx] ))
                {
                    LogError( L"CloseHandle failed for threads" );
                    return false;
                }
            }
            // Clear memory allocated for threads.
            delete[] m_phThreadList;
            m_phThreadList = 0;
            // Close the semaphore
            if( TRUE != CloseHandle( m_hSemaphore ))
            {
                LogError( L"CloseHandle failed for semaphore" );
                return false;
            }
            // Clear request queue.
            m_RequestQueue.clear();
            return true;
        }
        catch( ... )
        {
            LogError( L"Exception occurred in DestroyPool()" );
            return false;
        }
    }


    /** 
     * Check for destroy request.
     */
    inline bool ThreadPool::IsDestroyed()
    {
        // Avoid synchronization issues if destroy requested after validation.
        AutoLock LockThread( m_LockWorkerThread );
        // During thread pool destruction all semaphores are released
        // to exit all threads.
        return m_bDestroyed;
    }


    /** 
     * Set destroy flag
     */
    inline void ThreadPool::SetDestroyFlag( const bool bFlag_i )
    {
        AutoLock LockThread( m_LockWorkerThread );
        m_bDestroyed = bFlag_i;
    }


    /** 
     * Cancel all processing request in pool.
     */
    void ThreadPool::CancelRequests()
    {
        try
        {
            // Avoid synchronization issues if destroy requested after validation.
            AutoLock LockThread( m_LockWorkerThread );
            LogInfo( L"Thread pool destroy requested" );
            // Clear main queue.
            m_RequestQueue.clear();
        }
        catch( ... )
        {
            LogError( L"Exception occurred in CancelRequests()" );
        }
    }


    /** 
     * Log error in thread pool.
     * 
     * @param       wstrError_i - Error description.
     */
    void ThreadPool::LogError( const std::wstring& wstrError_i )
    {
        if( NULL != m_pLogger )
        {
            m_pLogger->LogError( m_lActiveThread, wstrError_i );
        }
    }


    /** 
     * Log information in thread pool.
     * 
     * @param       wstrInfo_i - Information description.
     */
    void ThreadPool::LogInfo( const std::wstring& wstrInfo_i )
    {
        if( NULL != m_pLogger )
        {
            m_pLogger->LogInfo( m_lActiveThread, wstrInfo_i );
        }
    }


    /** 
     * worker thread procedure.
     * 
     * @param       pParam_i - ThreadPool instance.
     * @return      UINT      - Return 0 on success.
     */
    UINT ThreadPool::ThreadProc( LPVOID pParam_i )
    {
        ThreadPool* pThreadPool = NULL;
        try
        {
            ThreadPool* pThreadPool = reinterpret_cast<ThreadPool*>( pParam_i );
            if( NULL == pThreadPool )
            {
                return 1;
            }
            if( !pThreadPool->ProcessRequests())
            {
                pThreadPool->LogError( L"ProcessRequests() failed" );
                return 1;
            }
            return 0;
        }
        catch( ... )
        {
            if( NULL !=  pThreadPool )
            {
                pThreadPool->LogError( L"Exception occurred in ThreadProc()" );
            }
            return 1;
        }
    }
} // namespace TP