//===== Copyright � 2009, Valve Corporation, All rights reserved. ======//
// filesyste_newasync.h
//
// Purpose: 
//
// $NoKeywords: $async file system
//===========================================================================//

#include <limits.h>

#include "tier0/threadtools.h"
#include "tier0/memalloc.h"
#include "tier1/interface.h"
#include "tier1/utlsymbol.h"
#include "appframework/iappsystem.h"
#include "tier1/checksum_crc.h"
#include "filesystem/iasyncfilesystem.h"

#ifndef FILESYSTEMASYNC_H
#define FILESYSTEMASYNC_H

#ifdef _WIN32
	#pragma once
#endif

#ifdef OSX
#pragma GCC diagnostic ignored "-Wreturn-type"			// control reaches end of non-void function, for unsupported assignment operators below
#endif

class CAsyncGroupRequest;


struct CAsyncResultInfo_t
{
	public:
		void*				m_pAllocatedBuffer;
		int					m_nBytesTransferred;
		FSAsyncStatus_t		m_ErrorCode;
		
		CAsyncResultInfo_t() : m_pAllocatedBuffer(nullptr), m_nBytesTransferred(0) {}
};


//-----------------------------------------------------------------------------
// CAsyncRequestBase - Common items to all async filesystem requests
//
// Allows the async filesystem to have a standard way to deal with different
// types of requests equally re: priority, callbacks, state, aborting, etc
//	
//-----------------------------------------------------------------------------
class CAsyncRequestBase
{

	public:
	
		void*							m_pOuter;					// pointer to object containing this base
	
		// -------------------------------------------------------
		// Members to set by the user before submission

		CFunctor*						m_pCallback;				// Optional Callback object (we own this)
		CIOCompletionQueue*				m_pResultQueue;				// Message Queue object to send results to
		
		AsyncFileOperation_t			m_Operation;				// operation to perform
		int32							m_priority;					// inter list priority, 0=lowest??

		// -------------------------------------------------------
		// Used internally by the filesystem to keep track of the request
		
		CAsyncRequestBase*				m_pNext;					// used to maintain the priority linked list
		CAsyncRequestBase*				m_pPrev;	

		// -------------------------------------------------------
		// Members which get set by the file system after submission

		AsyncRequestState_t				m_RequestState;				// State of this request (which stage it is n)
		AsyncRequestStatus_t			m_RequestStatus;			// Error/Success Status from the last operation attempted on this request
		
		bool							m_bAbortRequest;			// Flag indicating this request is to be aborted
		bool							m_bProcessingCallback;		// Flag indicating a callback is being processed
		bool							m_bDontAutoRelease;			// Flag indicating not to automatically release the request once the callback finishes
//		bool							m_bIsAContainer
		
		CThreadEvent*					m_pSyncThreadEvent;			// Thread event to signal in synchronous mode...
		
		FSAsyncControl_t				m_pOldAsyncControl;			// old Async System control Handle
		FSAsyncStatus_t					m_pOldAsyncStatus;			// old Async System IO Status
		
		CAsyncGroupRequest*				m_pGroup;					// group this request belongs to


	public:
										CAsyncRequestBase();		// Default Constructor - We want NO implicit constructions
										~CAsyncRequestBase();		// Destructor - not virtual anymore

		void							SetOuter( void* pOuter )			{ m_pOuter = pOuter; }
		void*							GetOuter() const { return m_pOuter; }

		IAsyncRequestBase*				GetInterfaceBase() const;

		// functions to query operation
		AsyncFileOperation_t			GetAsyncOperationType() const { return m_Operation; }

		// Set completion options								
		void							AssignCallback( CFunctor* pCallback );					// Add a completion callback to this request
		void							AssignResultQueue( CIOCompletionQueue* pMsgQueue );		// Send the results to this queue object
		void							AssignCallbackAndQueue( CIOCompletionQueue* pMsgQueue, CFunctor* pCallback );

		// Completion processing functions		
		void							ProcessCallback( bool bRelease = true );	// Execute the optional callback functor

		void							KeepRequestPostCallback()			{ m_bDontAutoRelease = true; }		// User wants to keep request alive after the callback completes
		void							DontKeepRequestPostCallback()		{ m_bDontAutoRelease = false; }		// User doesn't want to keep request after the callback completes 

		void							Release() const;							// lets user manually release the async request (only valid when completed)
		void							DeleteOuter() const;						// Used by async filesystem to delete the containing object (as well as the CAsyncRequestBase)

		// Priority Functions
		void							SetPriority( int32 nPriority );
		int32							GetPriority() const { return m_priority; }

		// Status & Results functions
		AsyncRequestState_t				GetRequestState() const { return m_RequestState; }
		AsyncRequestStatus_t			GetRequestStatus() const { return m_RequestStatus; }
		
		// --------------------------------------------------------------
		// Gateway to outer's Public Methods not included in the interface, used by Async FileSystem
		
		void							AbortAfterServicing( CAsyncResultInfo_t& results ) const;								// Handle a request that got aborted while being serviced
		void							UpdateAfterServicing( CAsyncResultInfo_t& results ) const;							// Handle a request that got aborted while being serviced
		AsyncRequestStatus_t			ValidateSubmittedRequest( bool bPerformSync ) const;									// Validates the derived object data at request submission
		bool							ValidateRequestModification() const;													// core check to determine if modification is allowed at this time
		
	protected:
		// Helper functions for repeated operations
		void							RefreshCallback( CFunctor* pCallback );

	private:

		explicit						CAsyncRequestBase( const CAsyncRequestBase& rhs );	// copy constructor - NOT SUPPORTED
		CAsyncRequestBase&				operator=( const CAsyncRequestBase& rhs );				// assignment operator - NOT SUPPORTED	
									

};




//-----------------------------------------------------------------------------
// CAsyncGroupRequest - Concrete implementation for a group of async requests
//
// 
//	
//-----------------------------------------------------------------------------
class CAsyncGroupRequest : public IAsyncGroupRequest
{
	public:
	
		CAsyncRequestBase				m_Base;							// Base request - we can't derive from due to multiple inheritance problems
		
		CUtlVector<IAsyncRequestBase*>	m_RequestList;					// List of requests to process
		CUtlVector<bool>				m_ValidList;					// List of requests which are valid
		
		CInterlockedInt					m_nNumRequestsOutstanding;
		


	public:
	
										CAsyncGroupRequest();			// Default Constructor - We want NO implicit constructions
		virtual							~CAsyncGroupRequest();			// Destructor

		// ---------------------------------------------------------
		// methods from the IAsyncRequestBase Interface
		// ---------------------------------------------------------

		// functions to query operation
		AsyncFileOperation_t	GetAsyncOperationType() override { return m_Base.GetAsyncOperationType(); }

		// Set completion options								
		void					AssignCallback( CFunctor* pCallback ) override {  m_Base.AssignCallback( pCallback ); }
		void					AssignResultQueue( CIOCompletionQueue* pMsgQueue ) override {  m_Base.AssignResultQueue( pMsgQueue ); }
		void					AssignCallbackAndQueue( CIOCompletionQueue* pMsgQueue, CFunctor* pCallback ) override {  m_Base.AssignCallbackAndQueue( pMsgQueue, pCallback); }

		// Completion processing functions		
		void					ProcessCallback( bool bRelease = true ) override {  m_Base.ProcessCallback( bRelease ); }

		void					KeepRequestPostCallback() override {  m_Base.KeepRequestPostCallback(); }
		void					DontKeepRequestPostCallback() override {  m_Base.DontKeepRequestPostCallback(); }

		void					Release() override {  m_Base.Release(); }

		// Priority Functions
		void					SetPriority( int32 nPriority ) override {  m_Base.SetPriority( nPriority ); }
		int32					GetPriority() override { return  m_Base.GetPriority(); }

		// Status & Results functions
		AsyncRequestState_t		GetRequestState() override { return  m_Base.GetRequestState(); }
		AsyncRequestStatus_t	GetRequestStatus() override { return  m_Base.GetRequestStatus(); }

	private:
		CAsyncRequestBase*		GetBase() override { return (CAsyncRequestBase*) &m_Base; }

		// ----------------------------------------------------
		// methods from the IAsyncGroupRequest Interface
		// ----------------------------------------------------

	public:
		void					AddAsyncRequest( IAsyncRequestBase* pRequest ) override;

		int32					GetAsyncRequestCount() override { return m_RequestList.Count(); }
		IAsyncRequestBase*		GetAsyncRequest( int32 nRNum ) override;
		IAsyncFileRequest*		GetAsyncFileRequest( int32 nRNum ) override;
		IAsyncSearchRequest*	GetAsyncSearchRequest( int32 nRNum ) override;

		void					ReleaseAsyncRequest( int32 nRNum ) override;

		void							NotifyOfCompletion(  IAsyncRequestBase* pRequest );

	private:

		explicit						CAsyncGroupRequest( const CAsyncRequestBase& rhs );		// copy constructor - NOT SUPPORTED
		CAsyncRequestBase&				operator=( const CAsyncGroupRequest& rhs );				// assignment operator - NOT SUPPORTED	

};













//-----------------------------------------------------------------------------
// CAsyncFileRequest - Concrete implementation for the IAsyncFileRequest interface
//
//   Manages a read or write to file request
//	
//-----------------------------------------------------------------------------
class CAsyncFileRequest : public IAsyncFileRequest
{
	
	public:
	
		CAsyncRequestBase				m_Base;							// Base request - we can't derive from due to multiple inheritance problems
	
		// -------------------------------------------------------
		// Members to set by the user before submission
		
		const char*						m_pFileName;					// file system name (copy of string that we own)
		
		void*							m_pUserProvidedDataBuffer;		// optional (if read op), system will alloc one if NULL/free if NULL
		size_t							m_nUserProvidedBufferSize;		// size of the memory buffer			
		
		int64							m_nFileSeekOffset;				// (optional) position in the file to seek to/begin xfer at, 0=beginning
		int64							m_nMaxIOSizeInBytes;			// optional read clamp, 0=full read

		// -------------------------------------------------------
		// Members which get set by the file system after submission	

		void*							m_pResultsBuffer;				// Buffer results were placed in, could be from user (or not)
		size_t							m_nResultsBufferSize;			// size of results buffer
		size_t							m_nIOActualSize;				// actual size of IO performed
		
		bool							m_bDeleteBufferMemory;			// Flag indicating the memory holding the file data is deleted when the request is released
		
	public:
									
										CAsyncFileRequest();			// Default Constructor - We want NO implicit constructions
		virtual							~CAsyncFileRequest();			// Destructor

		// ---------------------------------------------------------
		// methods from the IAsyncRequestBase Interface
		// ---------------------------------------------------------

		// functions to query operation
		AsyncFileOperation_t	GetAsyncOperationType() override { return m_Base.GetAsyncOperationType(); }

		// Set completion options								
		void					AssignCallback( CFunctor* pCallback ) override {  m_Base.AssignCallback( pCallback ); }
		void					AssignResultQueue( CIOCompletionQueue* pMsgQueue ) override {  m_Base.AssignResultQueue( pMsgQueue ); }
		void					AssignCallbackAndQueue( CIOCompletionQueue* pMsgQueue, CFunctor* pCallback ) override {  m_Base.AssignCallbackAndQueue( pMsgQueue, pCallback); }

		// Completion processing functions		
		void					ProcessCallback( bool bRelease = true ) override {  m_Base.ProcessCallback( bRelease ); }

		void					KeepRequestPostCallback() override {  m_Base.KeepRequestPostCallback(); }
		void					DontKeepRequestPostCallback() override {  m_Base.DontKeepRequestPostCallback(); }

		void					Release() override {  m_Base.Release(); }

		// Priority Functions
		void					SetPriority( int32 nPriority ) override {  m_Base.SetPriority( nPriority ); }
		int32					GetPriority() override { return  m_Base.GetPriority(); }

		// Status & Results functions
		AsyncRequestState_t		GetRequestState() override { return  m_Base.GetRequestState(); }
		AsyncRequestStatus_t	GetRequestStatus() override { return  m_Base.GetRequestStatus(); }

	private:
		CAsyncRequestBase*		GetBase() override { return (CAsyncRequestBase*) &m_Base; }

		// ----------------------------------------------------
		// methods from the IAsyncFileRequest Interface
		// ----------------------------------------------------

	public:	
		// functions to set filename and operation
		void					LoadFile( const char* pFileName ) override;									// make this a 'read data from file' request
		void					SaveFile( const char* pFileName ) override;									// make this a 'write data to file' request
		void					AppendFile( const char* pFileName ) override;								// make this a 'append data to file' request

		void					SetFileName( const char* pFileName ) override;								// assign the filename to use
		const char*				GetFileName() override { return m_pFileName; }					// get the filename we've assigned
		
		// Buffer control functions		
		void					SetUserBuffer( void* pDataBuffer, size_t nBufferSize ) override;				// User supplies a memory buffer to use, which they own the memory for
		void*					GetUserBuffer() override { return m_pUserProvidedDataBuffer; }	// returns the address of the user supplied buffer
		size_t					GetUserBufferSize() override { return m_nUserProvidedBufferSize; }	// returns the size of the user supplied buffer

		void					ProvideDataBuffer() override;												// file system provide a buffer with the results
		
		// returned buffer (read) functions		
		void*					GetResultBuffer() override { return m_pResultsBuffer; }			// Returns the address of the data transferred
		size_t					GetResultBufferSize() override { return m_nResultsBufferSize; }		// Returns the size of the buffer holding the data transferred
		size_t					GetIOTransferredSize() override { return m_nIOActualSize; }				// Returns the number of bytes of data actually transferred

		// Memory control functions for buffers allocated by the async file system
		void					KeepResultBuffer() override { m_bDeleteBufferMemory = false; }		// User wants to keeps buffer allocated by the file system
		void					ReleaseResultBuffer() override { m_bDeleteBufferMemory = true; }		// User decides they want the request to take care of releasing the buffer

		// file position functions										
		void					ReadFileDataAt( int64 nOffset, size_t nReadSize = 0 ) override;				// Read file data starting at supplied offset, optional max size to read
		void					WriteFileDataAt( int64 nOffset, size_t nWriteSize = 0 ) override;			// Write data to file at supplied offset, optional size to write (max size of buffer)
	

		// --------------------------------------------------------------
		// Public Methods not included in the interface, used by Async FileSystem
		
		void							AbortAfterServicing( CAsyncResultInfo_t& results );								// Handle a request that got aborted while being serviced
		void							UpdateAfterServicing( CAsyncResultInfo_t& results );							// Handle a request that got aborted while being serviced
		AsyncRequestStatus_t			ValidateSubmittedRequest( bool bPerformSync );									// Validates the derived object data at request submission
		bool							ValidateRequestModification() const;													// Check to determine if modification is allowed at this time
	
	protected:
		// Helper functions for repeated operations
		void							RefreshFileName( const char* pNewFileName );

	private:
		// disabled functions
		explicit						CAsyncFileRequest( const CAsyncFileRequest& rhs );				// copy constructor - NOT SUPPORTED
		CAsyncFileRequest&				operator=( const CAsyncFileRequest& rhs );						// assignment operator - NOT SUPPORTED	
									
};



//-----------------------------------------------------------------------------
// CAsyncSearchRequest - Implementation of the Interface for setting and reading 
//                       the results of an asynchronous directory search request
//	
//-----------------------------------------------------------------------------
class CAsyncSearchRequest  :  public IAsyncSearchRequest
{

	public:
		CAsyncRequestBase					m_Base;						// Base request - we can't derive from due to multiple inheritance problems
	
//		CUtlStringList						m_DirectoryList;			// List of directories we have scanned
		
		int32								m_nNumResults;				// number of results 
		bool								m_bRecurseSubdirs;			// flag to activate recursing of subdirectories
		
		CUtlVector<CDirectoryEntryInfo_t>	m_Results;					// Search results...

		char								m_FileSpec[MAX_PATH];		// filespec of the files we are looking for
		char								m_PathID[MAX_PATH];
		char								m_SearchPath[MAX_PATH];
		
		char								m_SearchSpec[MAX_PATH];

	public:
	
										CAsyncSearchRequest();						// Default Constructor - We want NO implicit constructions
		virtual							~CAsyncSearchRequest();						// Destructor

		// ---------------------------------------------------------
		// methods from the IAsyncRequestBase Interface
		// ---------------------------------------------------------
		
		// functions to query operation
		AsyncFileOperation_t	GetAsyncOperationType() override { return m_Base.GetAsyncOperationType(); }

		// Set completion options								
		void					AssignCallback( CFunctor* pCallback ) override {  m_Base.AssignCallback( pCallback ); }
		void					AssignResultQueue( CIOCompletionQueue* pMsgQueue ) override {  m_Base.AssignResultQueue( pMsgQueue ); }
		void					AssignCallbackAndQueue( CIOCompletionQueue* pMsgQueue, CFunctor* pCallback ) override {  m_Base.AssignCallbackAndQueue( pMsgQueue, pCallback); }

		// Completion processing functions		
		void					ProcessCallback( bool bRelease = true ) override {  m_Base.ProcessCallback( bRelease ); }

		void					KeepRequestPostCallback() override {  m_Base.KeepRequestPostCallback(); }
		void					DontKeepRequestPostCallback() override {  m_Base.DontKeepRequestPostCallback(); }

		void					Release() override {  m_Base.Release(); }

		// Priority Functions
		void					SetPriority( int32 nPriority ) override {  m_Base.SetPriority( nPriority ); }
		int32					GetPriority() override { return  m_Base.GetPriority(); }

		// Status & Results functions
		AsyncRequestState_t		GetRequestState() override { return  m_Base.GetRequestState(); }
		AsyncRequestStatus_t	GetRequestStatus() override { return  m_Base.GetRequestStatus(); }

	private:
		CAsyncRequestBase*		GetBase() override { return (CAsyncRequestBase*) &m_Base; }

		// ---------------------------------------------------------
		// methods from the IAsyncSearchRequest Interface
		// ---------------------------------------------------------

	public:
		// functions to define the request

		void					SetSearchFilespec( const char* pFullSearchSpec ) override;
		void					SetSearchPathAndFileSpec( const char* pPathId, const char* pRelativeSearchSpec ) override;
		void					SetSearchPathAndFileSpec( const char* pPathId, const char* pRelativeSearchPath, const char* pSearchSpec ) override;

		void					SetSubdirectoryScan( const bool bInclude ) override;
		bool					GetSubdirectoryScan() override { return m_bRecurseSubdirs; }
		
		// Functions to return the results.

		int						GetResultCount() override { return m_nNumResults; }
		CDirectoryEntryInfo_t*	GetResult( int rNum = 0 ) override;
		const char*				GetMatchedFile( int rNum = 0 ) override;

		// --------------------------------------------------------------
		// Public Methods not included in the interface, used by Async FileSystem
		
		void							AbortAfterServicing( CAsyncResultInfo_t& results );									// Handle a request that got aborted while being serviced
		void							UpdateAfterServicing( CAsyncResultInfo_t& results );								// Handle a request that got aborted while being serviced
		AsyncRequestStatus_t			ValidateSubmittedRequest( bool bPerformSync ) const;										// Validates the derived object data at request submission
		bool							ValidateRequestModification() const;														// Check to determine if modification is allowed at this time

	private:
		// disabled functions
		explicit						CAsyncSearchRequest( const CAsyncSearchRequest& rhs );	// copy constructor - NOT SUPPORTED
		CAsyncSearchRequest&			operator=( const CAsyncSearchRequest& rhs );			// assignment operator - NOT SUPPORTED	

};


//-----------------------------------------------------------------------------
// CAsyncRequestQueue
//		Helper object for filesystem - Linked list of objects
//      dreeived from CAsyncRequestBase
//-----------------------------------------------------------------------------
class CAsyncRequestQueue
{
	private:
		CThreadFastMutex			m_Mutex;
		int32						m_nQueueSize;
		CAsyncRequestBase*			m_pHead;
		CAsyncRequestBase*			m_pTail;

	public:
		CAsyncRequestQueue();
		~CAsyncRequestQueue();

		int32						GetSize() const { return m_nQueueSize; }
		bool						IsInQueue( CAsyncRequestBase* pItem ) const;
		bool						IsInQueueIp( const IAsyncRequestBase* pInterfaceBase ) const;
		
		void						AddToHead( CAsyncRequestBase* pItem );
		void						AddToTail( CAsyncRequestBase* pItem );
		void						InsertBefore( CAsyncRequestBase* pItem, CAsyncRequestBase* pInsertAt );
		void						InsertAfter( CAsyncRequestBase* pItem, CAsyncRequestBase* pInsertAt );
									
		void						PriorityInsert( CAsyncRequestBase* pItem );
									
		void						Remove( CAsyncRequestBase* pItem );
		CAsyncRequestBase*			RemoveHead();

};


//-----------------------------------------------------------------------------
// CAsyncFileSystem - implements IAsyncFileSystem interface
//	
//-----------------------------------------------------------------------------
class CAsyncFileSystem : public CTier2AppSystem< IAsyncFileSystem >
{
	typedef  CTier2AppSystem< IAsyncFileSystem >  BaseClass;
	
	friend class CAsyncGroupRequest;
	
	public:
	
	static const int				s_MaxPlatformFileNameLength = 40;
	
	public:
		CAsyncFileSystem();
		~CAsyncFileSystem();
											
		//--------------------------------------------------
		// Interface methods exposed with IAsyncFileSystem
		//--------------------------------------------------

	AsyncRequestStatus_t			SubmitAsyncFileRequest( const IAsyncRequestBase* pRequest ) override;
	AsyncRequestStatus_t			SubmitSyncFileRequest( const IAsyncRequestBase* pRequest ) override;

	AsyncRequestStatus_t			GetAsyncFileRequestStatus( const IAsyncRequestBase* pRequest ) override;
	AsyncRequestStatus_t			AbortAsyncFileRequest( const IAsyncRequestBase* pRequest ) override;

	void							SuspendAllAsyncIO( bool bWaitForIOCompletion = true ) override;
	void							ResumeAllAsyncIO() override;
	AsyncRequestStatus_t			AbortAllAsyncIO( bool bWaitForIOCompletion = true ) override;

	IAsyncFileRequest*				CreateNewFileRequest() override;
	IAsyncSearchRequest*			CreateNewSearchRequest() override;
	IAsyncGroupRequest*				CreateNewAsyncRequestGroup() override;

	void							ReleaseAsyncRequest( const IAsyncRequestBase* pRequest ) override;
	bool							BlockUntilAsyncIOComplete( const IAsyncRequestBase* pRequest ) override;

	void*							AllocateBuffer( size_t nBufferSize, int nAlignment = 4 ) override;
	void							ReleaseBuffer( void* pBuffer ) override;

		//--------------------------------------------------
		// Public Methods used inside the filesystem
		//--------------------------------------------------
		
	
		void*								QueryInterface( const char *pInterfaceName ) override;
		
		void								RemoveRequest( CAsyncRequestBase* pRequest );

		bool								ResolveAsyncRequest( const IAsyncRequestBase* pRequest, CAsyncRequestBase*& pRequestBase, AsyncRequestState_t& CurrentStage ) const;

		bool								ValidateRequestPtr( CAsyncRequestBase* pRequest ) const;

	private:
		CAsyncRequestQueue					m_Composing;				// requests in state: Composing
		CAsyncRequestQueue					m_Submitted;				// requests in state: Submitted
		CAsyncRequestQueue					m_InFlight;					// requests in state: Serviced
		CAsyncRequestQueue					m_Completed;				// requests in state: Awaiting Finished, Completed*

		
		CInterlockedInt						m_nJobsInflight;			// number of requests currently being serviced by the old system
		CInterlockedInt						m_nSuspendCount;			// number of requests to suspend all Async IO outstanding (nested request support)
		bool								m_bIOSuspended;

#if defined(_DEBUG)
		CThreadFastMutex					m_MemoryTrackMutex;
		CUtlVector<void*>					m_AllocList;				// Debug build - track allocations
#endif

		CThreadFastMutex					m_AsyncStateUpdateMutex;
		CThreadEvent						m_CompletionSignal;			// Used to signal the completion of an IO

		AsyncRequestStatus_t				ValidateRequest( CAsyncRequestBase* pRequest, bool bPerformSync = false ) const;

		void								KickOffFileJobs();			
		void								WaitForServincingIOCompletion() const;	// Pauses until all jobs being serviced are complete

		static void*						OldAsyncAllocatorCallback( const char *pszFilename, unsigned nBytes );
		
		static void							AsyncIOCallbackGateway(const FileAsyncRequest_t &request, int nBytesRead, FSAsyncStatus_t err);
		static void							AsyncSearchCallbackAddItem( void* pContext, char* pFoundPath, char* pFoundFile );
		static void							AsyncSearchCallbackGateway( void* pContext, FSAsyncStatus_t err );
		
		void								AsyncIOCallBackHandler(CAsyncRequestBase* pRequest, CAsyncResultInfo_t& results );
			
		void								NotifyMessageQueueOrCallback( CAsyncRequestBase* pRequest );
			
			
};



//-----------------------------------------------------------------------------
// Utility functions
//	
//-----------------------------------------------------------------------------
AsyncRequestStatus_t ValidateFileName( const char* pFileName );



#endif
