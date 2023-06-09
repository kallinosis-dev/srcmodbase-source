//===== Copyright 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef BASEFILESYSTEM_H
#define BASEFILESYSTEM_H

#include "tier0/platform.h"

#ifdef _PS3
#include <sdk_version.h>
	#if CELL_SDK_VERSION >= 0x085007
		#define getenv(x) NULL   //TEMP REMOVE THIS - RP
	#endif
#endif
#ifdef _WIN32
#pragma once
#endif

#if defined( _WIN32 )

#if !defined( _X360 ) && !defined( _PS3 ) && !defined(LINUX)
	#include <io.h>
	#include <direct.h>
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#define SUPPORT_VPK
#endif
#undef GetCurrentDirectory
#undef GetJob
#undef AddJob


#elif defined( POSIX ) && !defined( _PS3 )
	#include <unistd.h> // unlink
	#include "linux_support.h"
	#define INVALID_HANDLE_VALUE (void *)-1

	// undo the prepended "_" 's
	#define _chmod chmod
	#define _stat stat
	#define _alloca alloca
	#define _S_IFDIR S_IFDIR
	
	#define SUPPORT_VPK


#elif defined(_PS3)
#include <unistd.h> // unlink
#define INVALID_HANDLE_VALUE ( void * )-1

// undo the prepended "_" 's
#define _chmod chmod
#define _stat stat
#define _alloca alloca
#define _S_IFDIR S_IFDIR

#endif

#include "tier0/threadtools.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _PS3
#include <malloc.h>
#endif // _PS3
#include <string.h>
#include "tier1/utldict.h"

#ifdef SUPPORT_VPK
#include "vpklib/packedstore.h"
#endif
#include <time.h>
#include "refcount.h"
#include "filesystem.h"
#include "tier1/utlvector.h"
#include "tier1/UtlStringMap.h"
#include <stdarg.h>
#include "tier1/utlrbtree.h"
#include "tier1/utlsymbol.h"
#include "tier1/utllinkedlist.h"
#include "tier1/utlstring.h"
#include "tier1/utlsortvector.h"
#include "bspfile.h"
#include "tier1/utldict.h"
#include "tier1/tier1.h"
#include "byteswap.h"
#include "threadsaferefcountedobject.h"
#include "filetracker.h"

#ifdef _PS3
#include "ps3/ps3_platform.h"
#include "ps3/ps3_core.h"
#else
#include "xbox/xboxstubs.h"
#endif // _PS3

#include "tier0/memdbgon.h"

#ifdef _WIN32
#define CORRECT_PATH_SEPARATOR '\\'
#define INCORRECT_PATH_SEPARATOR '/'
#elif defined(POSIX)
#define CORRECT_PATH_SEPARATOR '/'
#define INCORRECT_PATH_SEPARATOR '\\'
#endif

#ifdef	_WIN32
#define PATHSEPARATOR(c) ((c) == '\\' || (c) == '/')
#elif defined(POSIX)
#define PATHSEPARATOR(c) ((c) == '/')
#endif	//_WIN32

#define MAX_FILEPATH 512 

#if !defined( _X360 )
#define SUPPORT_IODELAY_MONITORING
#endif

#ifdef _PS3
#define FILE_ATTRIBUTE_DIRECTORY S_IFDIR

typedef struct 
{
	// public data
	int dwFileAttributes;
	char cFileName[PATH_MAX]; // the file name returned from the call

	int numMatches;
	struct dirent **namelist;  
} FIND_DATA;

#define WIN32_FIND_DATA FIND_DATA

#endif // _PS3

extern CUtlSymbolTableMT g_PathIDTable;

struct DLCCorrupt_t
{
	XCONTENT_DATA	m_ContentData;
};

struct DLCContent_t
{
	XCONTENT_DATA	m_ContentData;
	DWORD			m_LicenseMask;
	int				m_nController;
	char			m_szVolume[8];
	bool			m_bMounted;
	bool			m_bCorrupt;
};

class CDLCLess
{
public:
	bool Less( const DLCContent_t& lhs, const DLCContent_t& rhs, void *pContext )
	{
		return ( ( lhs.m_LicenseMask & 0xFFFF0000 ) < ( rhs.m_LicenseMask & 0xFFFF0000 ) );
	}
};

enum FileMode_t
{
	FM_BINARY,
	FM_TEXT
};

enum FileType_t
{
	FT_NORMAL,
	FT_PACK_BINARY,
	FT_PACK_TEXT,
#if defined(_PS3)
    FT_RUNTIME_PS3,
#endif
};

#if defined(_PS3)
void FixUpPathCaseForPS3( const char* pFilePath );
#endif

class IThreadPool;
class CAsyncJobFuliller;
class CBlockingFileItemList;
class KeyValues;
class CCompiledKeyValuesReader;
class CBaseFileSystem;
class CPackFileHandle;
class CPackFile;
class IFileList;
class CFileOpenInfo;
class CFileHandleTimer;

class CWhitelistSpecs
{
public:
	IFileList	*m_pWantCRCList;
	IFileList	*m_pAllowFromDiskList;
};
typedef CThreadSafeRefCountedObject<CWhitelistSpecs*> CWhitelistHolder;

//-----------------------------------------------------------------------------

class CIODelayAlarmThread;

class CFileHandle
{
public:
	CFileHandle( CBaseFileSystem* fs );
	virtual ~CFileHandle();

	void	Init( CBaseFileSystem* fs );

	int		GetSectorSize() const;
	bool	IsOK() const;
	void	Flush() const;
	void	SetBufferSize( int nBytes ) const;

	int		Read( void* pBuffer, int nLength );
	int		Read( void* pBuffer, int nDestSize, int nLength );

	int		Write( const void* pBuffer, int nLength ) const;
	int		Seek( int64 nOffset, int nWhence );
	int		Tell() const;
	int		Size() const;

	int64 AbsoluteBaseOffset() const;
	bool	EndOfFile() const;

	char *m_pszTrueFileName;
	char const *Name() const { return m_pszTrueFileName ? m_pszTrueFileName : ""; }

	void SetName( char const *pName )
	{
		Assert( pName );
		Assert( !m_pszTrueFileName );
		int len = Q_strlen( pName );
		m_pszTrueFileName = new char[len + 1];
		memcpy( m_pszTrueFileName, pName, len + 1 );
	}

	CPackFileHandle		*m_pPackFileHandle;
#ifdef SUPPORT_VPK
	CPackedStoreFileHandle m_VPKHandle;
#endif
	int64				m_nLength;
	FileType_t			m_type;
	FILE				*m_pFile;

protected:
	CBaseFileSystem		*m_fs;

#ifdef _PS3
// FOURCCs generate a warning on PS3
	enum
	{
		MAGIC = 0xABCDABCD,
		FREE_MAGIC = 0xDEADBEEF
	};
#else // !_PS3
	enum
	{
		MAGIC = 'CFHa',
		FREE_MAGIC = 'FreM'
	};
#endif // _PS3
	unsigned int	m_nMagic;

	bool IsValid() const;
};

// A pack file handle - essentially represents a file inside the pack file.  
// Note, there is no provision for compression here at the current time.
class CPackFileHandle
{
public:
	inline CPackFileHandle( CPackFile* pOwner, int64 nBase, unsigned int nLength, unsigned int nIndex = -1, unsigned int nFilePointer = 0 );
	inline ~CPackFileHandle();

	int				Read( void* pBuffer, int nDestSize, int nBytes );
	int				Seek( int nOffset, int nWhence );
	int				Tell() const { return m_nFilePointer; }
	int				Size() const { return m_nLength; }

	inline void		SetBufferSize( int nBytes ) const;
	inline int		GetSectorSize() const;
	inline int64	AbsoluteBaseOffset() const;

protected:
	int64			m_nBase;			// Base offset of the file inside the pack file.
	unsigned int	m_nFilePointer;		// Current seek pointer (0 based from the beginning of the file).
	CPackFile*		m_pOwner;			// Pack file that owns this handle
	unsigned int	m_nLength;			// Length of this file.
	unsigned int	m_nIndex;			// Index into the pack's directory table
};

//-----------------------------------------------------------------------------

class CPackFile : public CRefCounted<CRefCountServiceMT>
{		
public:

	inline CPackFile();
	inline ~CPackFile() override;

	// The means by which you open files:
	virtual CFileHandle *OpenFile( const char *pFileName, const char *pOptions = "rb" );

	// The two functions a pack file must provide
	virtual bool Prepare( int64 fileLen = -1, int64 nFileOfs = 0 ) = 0;
	virtual bool FindFile( const char *pFilename,  int &nIndex, int64 &nPosition, int &nLength ) = 0;

	// This is the core IO routine for reading anything from a pack file, everything should go through here at some point
	virtual int ReadFromPack( int nIndex, void* buffer, int nDestBytes, int nBytes, int64 nOffset );
	
	// Returns the filename for a given file in the pack. Returns true if a filename is found, otherwise buffer is filled with "unknown"
	virtual bool IndexToFilename( int nIndex, char* buffer, int nBufferSize ) = 0;

	inline int GetSectorSize() const;

	virtual void SetupPreloadData() {}
	virtual void DiscardPreloadData() {}
	virtual int64 GetPackFileBaseOffset() = 0;

	virtual CRC32_t GetKVPoolKey() { return 0; }
	virtual bool GetStringFromKVPool( unsigned int key, char *pOutBuff, int buflen ) { return false; }

	// Note: threading model for pack files assumes that data
	// is segmented into pack files that aggregate files
	// meant to be read in one thread. Performance characteristics
	// tuned for that case
	CThreadFastMutex	m_mutex;

	// Path management:
	void SetPath( const CUtlSymbol &path ) { m_Path = path; }
	const CUtlSymbol& GetPath() const	{ Assert( m_Path != UTL_INVAL_SYMBOL ); return m_Path; }
	CUtlSymbol			m_Path;

	// possibly embedded pack
	int64				m_nBaseOffset;

	CUtlString			m_ZipName;

	bool				m_bIsMapPath;
	long				m_lPackFileTime;

	int					m_refCount;
	int					m_nOpenFiles;

	FILE				*m_hPackFileHandleFS;
#ifdef SUPPORT_VPK
	CPackedStoreFileHandle m_hPackFileHandleVPK;
#endif
	bool				m_bIsExcluded;

	int					m_PackFileID;
protected:
	int64				m_FileLength;
	CBaseFileSystem		*m_fs;

	friend class		CPackFileHandle;
};

class CZipPackFile : public CPackFile
{
public:
	CZipPackFile( CBaseFileSystem* fs, void *pSection = nullptr);
	~CZipPackFile() override;

	// Loads the pack file
	bool Prepare( int64 fileLen = -1, int64 nFileOfs = 0 ) override;
	bool FindFile( const char *pFilename, int &nIndex, int64 &nOffset, int &nLength ) override;
	int  ReadFromPack( int nIndex, void* buffer, int nDestBytes, int nBytes, int64 nOffset  ) override;

	int64 GetPackFileBaseOffset() override { return m_nBaseOffset; }

	bool	IndexToFilename( int nIndex, char *pBuffer, int nBufferSize ) override;

	CRC32_t GetKVPoolKey() override;
	bool GetStringFromKVPool( unsigned int key, char *pOutBuff, int buflen ) override;

protected:
	#pragma pack(1)

	typedef struct
	{
		char name[ 112 ];
		int64 filepos;
		int64 filelen;
	} packfile64_t;

	typedef struct
	{
		char id[ 4 ];
		int64 dirofs;
		int64 dirlen;
	} packheader64_t;

	typedef struct
	{
		char id[ 8 ];
		int64 packheaderpos;
		int64 originalfilesize;
	} packappenededheader_t;

	#pragma pack()

	// A Pack file directory entry:
	class CPackFileEntry
	{
	public:
		unsigned int		m_nPosition;
		unsigned int		m_nLength;
		unsigned int		m_HashName;
		unsigned short		m_nPreloadIdx;
		unsigned short		pad;
		FileNameHandle_t	m_hDebugFilename;
	};

	class CPackFileLessFunc
	{
	public:
		bool Less( CPackFileEntry const& src1, CPackFileEntry const& src2, void *pCtx );
	};

	// Find a file inside a pack file:
	const CPackFileEntry* FindFile( const char* pFileName );

	// Entries to the individual files stored inside the pack file.
	CUtlSortVector< CPackFileEntry, CPackFileLessFunc > m_PackFiles;

	bool						GetOffsetAndLength( const char *FileName, int &nBaseIndex, int64 &nFileOffset, int &nLength );

	// Preload Support
	void						SetupPreloadData() override;	
	void						DiscardPreloadData() override;	
	ZIP_PreloadDirectoryEntry*	GetPreloadEntry( int nEntryIndex );

	int64						m_nPreloadSectionOffset;
	unsigned int				m_nPreloadSectionSize;
	ZIP_PreloadHeader			*m_pPreloadHeader;
	unsigned short*				m_pPreloadRemapTable;
	ZIP_PreloadDirectoryEntry	*m_pPreloadDirectory;
	void*						m_pPreloadData;
	CByteswap					m_swap;

	CRC32_t						m_KVPoolKey;
	CUtlFilenameSymbolTable		m_KVStringPool;

	void						*m_pSection;
};

class CFileLoadInfo
{
public:
	bool	m_bSteamCacheOnly;			// If Steam and this is true, then the file is only looked for in the Steam caches.
	bool	m_bLoadedFromSteamCache;	// If Steam, this tells whether the file was loaded off disk or the Steam cache.
#ifdef _PS3
    Ps3FileType_t m_ps3Filetype;
#endif
};


//-----------------------------------------------------------------------------

#ifdef AsyncRead
#undef AsyncRead
#undef AsyncReadMutiple
#endif

//-----------------------------------------------------------------------------

abstract_class CBaseFileSystem : public CTier2AppSystem< IFileSystem >
{
	friend class CPackFileHandle;
	friend class CPackFile;
	friend class CXZipPackFile;	
	friend class CFileHandle;
	friend class CFileTracker;
	friend class CFileTracker2;
	friend class CFileOpenInfo;

	typedef CTier2AppSystem< IFileSystem > BaseClass;

public:
	CBaseFileSystem();
	~CBaseFileSystem();

	// Methods of IAppSystem
	void				*QueryInterface( const char *pInterfaceName ) override;
	InitReturnVal_t		Init() override;
	void				Shutdown() override;

	void						InitAsync();
	void						ShutdownAsync();

	void						ParsePathID( const char* &pFilename, const char* &pPathID, char tempPathID[MAX_PATH] ) const;

	// file handling
	FileHandle_t		Open( const char *pFileName, const char *pOptions, const char *pathID ) override;
	FileHandle_t		OpenEx( const char *pFileName, const char *pOptions, unsigned flags = 0, const char *pathID = nullptr, char **ppszResolvedFilename = nullptr) override;
	void				Close( FileHandle_t ) override;
	void				Seek( FileHandle_t file, int pos, FileSystemSeek_t method ) override;
	unsigned int		Tell( FileHandle_t file ) override;
	unsigned int		Size( FileHandle_t file ) override;
	unsigned int		Size( const char *pFileName, const char *pPathID ) override;

	void				SetBufferSize( FileHandle_t file, unsigned nBytes ) override;
	bool				IsOk( FileHandle_t file ) override;
	void				Flush( FileHandle_t file ) override;
	bool				Precache( const char *pFileName, const char *pPathID ) override;
	bool				EndOfFile( FileHandle_t file ) override;

	int					Read( void *pOutput, int size, FileHandle_t file ) override;
	int					ReadEx( void* pOutput, int sizeDest, int size, FileHandle_t file ) override;
	int					Write( void const* pInput, int size, FileHandle_t file ) override;
	char				*ReadLine( char *pOutput, int maxChars, FileHandle_t file ) override;
	int					FPrintf( FileHandle_t file, PRINTF_FORMAT_STRING const char *pFormat, ... ) override FMTFUNCTION( 3, 4 );

	// Reads/writes files to utlbuffers
	bool				ReadFile( const char *pFileName, const char *pPath, CUtlBuffer &buf, int nMaxBytes, int nStartingByte, FSAllocFunc_t pfnAlloc = nullptr) override;
	bool				WriteFile( const char *pFileName, const char *pPath, CUtlBuffer &buf ) override;
	bool				UnzipFile( const char *pFileName, const char *pPath, const char *pDestination ) override;
	int					ReadFileEx( const char *pFileName, const char *pPath, void **ppBuf, bool bNullTerminate, bool bOptimalAlloc, int nMaxBytes = 0, int nStartingByte = 0, FSAllocFunc_t pfnAlloc = nullptr) override;
	bool				ReadToBuffer( FileHandle_t hFile, CUtlBuffer &buf, int nMaxBytes = 0, FSAllocFunc_t pfnAlloc = nullptr) override;

	// Optimal buffer
	bool						GetOptimalIOConstraints( FileHandle_t hFile, unsigned *pOffsetAlign, unsigned *pSizeAlign, unsigned *pBufferAlign ) override;
	void						*AllocOptimalReadBuffer( FileHandle_t hFile, unsigned nSize, unsigned nOffset ) override { return malloc( nSize ); }
	void						FreeOptimalReadBuffer( void *p ) override { free( p ); }

	// Gets the current working directory
	bool				GetCurrentDirectory( char* pDirectory, int maxlen ) override;

	// this isn't implementable on STEAM as is.
	void				CreateDirHierarchy( const char *path, const char *pathID ) override;

	// returns true if the file is a directory
	bool				IsDirectory( const char *pFileName, const char *pathID ) override;

	// path info
	const char			*GetLocalPath( const char *pFileName, char *pLocalPath, int localPathBufferSize ) override;
	bool				FullPathToRelativePath( const char *pFullpath, char *pRelative, int maxlen ) override;

	// removes a file from disk
	void				RemoveFile( char const* pRelativePath, const char *pathID ) override;

	// Remove all search paths (including write path?)
	void				RemoveAllSearchPaths( void ) override;

	// Purpose: Removes all search paths for a given pathID, such as all "GAME" paths.
	void				RemoveSearchPaths( const char *pathID ) override;

	// STUFF FROM IFileSystem
	// Add paths in priority order (mod dir, game dir, ....)
	// Can also add pak files (errr, NOT YET!)
	void				AddSearchPath( const char *pPath, const char *pathID, SearchPathAdd_t addType ) override;
	bool				RemoveSearchPath( const char *pPath, const char *pathID ) override;
	void				PrintSearchPaths( void ) override;

	void				MarkPathIDByRequestOnly( const char *pPathID, bool bRequestOnly ) override;

	bool				IsFileInReadOnlySearchPath(const char *pPath, const char *pathID = nullptr) override;

	bool				FileExists( const char *pFileName, const char *pPathID = nullptr) override;
	long				GetFileTime( const char *pFileName, const char *pPathID = nullptr) override;
	bool				IsFileWritable( char const *pFileName, const char *pPathID = nullptr) override;
	bool				SetFileWritable( char const *pFileName, bool writable, const char *pPathID = nullptr ) override;
	void				FileTimeToString( char *pString, int maxChars, long fileTime ) override;

	const char			*FindFirst( const char *pWildCard, FileFindHandle_t *pHandle ) override;
	const char			*FindFirstEx( const char *pWildCard, const char *pPathID, FileFindHandle_t *pHandle ) override;
	const char			*FindNext( FileFindHandle_t handle ) override;
	bool				FindIsDirectory( FileFindHandle_t handle ) override;
	void				FindClose( FileFindHandle_t handle ) override;

	void				FindFileAbsoluteList( CUtlVector< CUtlString > &outAbsolutePathNames, const char *pWildCard, const char *pPathID ) override;

	void				PrintOpenedFiles( void ) override;
	void				SetWarningFunc( void (*pfnWarning)( const char *fmt, ... ) ) override;
	void				SetWarningLevel( FileWarningLevel_t level ) override;
	void				AddLoggingFunc( FileSystemLoggingFunc_t logFunc ) override;
	void				RemoveLoggingFunc( FileSystemLoggingFunc_t logFunc ) override;
	bool				RenameFile( char const *pOldPath, char const *pNewPath, const char *pathID ) override;

	void				GetLocalCopy( const char *pFileName ) override;

	FileNameHandle_t	FindOrAddFileName( char const *pFileName ) override;
	FileNameHandle_t	FindFileName( char const *pFileName ) override;
	bool				String( const FileNameHandle_t& handle, char *buf, int buflen ) override;
	int					GetPathIndex( const FileNameHandle_t &handle ) override;
	long						GetPathTime( const char *pFileName, const char *pPathID ) override;
	
	bool						ShouldGameReloadFile( const char *pFilename );
	void				EnableWhitelistFileTracking( bool bEnable, bool bCacheAllVPKHashes, bool bRecalculateAndCheckHashes ) override;
	void				RegisterFileWhitelist( IFileList *pWantCRCList, IFileList *pAllowFromDiskList, IFileList **pFilesToReload ) override;
	void				MarkAllCRCsUnverified() override;
	void				CacheFileCRCs( const char *pPathname, ECacheCRCType eType, IFileList *pFilter ) override;
	void						CacheFileCRCs_R( const char *pPathname, ECacheCRCType eType, IFileList *pFilter, CUtlDict<int,int> &searchPathNames );
	EFileCRCStatus		CheckCachedFileHash( const char *pPathID, const char *pRelativeFilename, int nFileFraction, FileHash_t *pFileHash ) override;
	int					GetUnverifiedFileHashes( CUnverifiedFileHash *pFiles, int nMaxFiles ) override;
	int					GetWhitelistSpewFlags() override;
	void				SetWhitelistSpewFlags( int flags ) override;
	void				InstallDirtyDiskReportFunc( FSDirtyDiskReportFunc_t func ) override;

	void				CacheAllVPKFileHashes( bool bCacheAllVPKHashes, bool bRecalculateAndCheckHashes ) override;
	bool				CheckVPKFileHash( int PackFileID, int nPackFileNumber, int nFileFraction, MD5Value_t &md5Value ) override;

	// Returns the file system statistics retreived by the implementation.  Returns NULL if not supported.
	const FileSystemStatistics *GetFilesystemStatistics() override;
	
	// GetVPKFileStatisticsKV
	void GetVPKFileStatisticsKV( KeyValues *pKV ) override;

	// Load dlls
	CSysModule 			*LoadModule( const char *pFileName, const char *pPathID, bool bValidatedDllOnly ) override;
	void				UnloadModule( CSysModule *pModule ) override;

	//--------------------------------------------------------
	// asynchronous file loading
	//--------------------------------------------------------
	FSAsyncStatus_t		AsyncReadMultiple( const FileAsyncRequest_t *pRequests, int nRequests, FSAsyncControl_t *pControls ) override;
	FSAsyncStatus_t		AsyncReadMultipleCreditAlloc( const FileAsyncRequest_t *pRequests, int nRequests, const char *pszFile, int line, FSAsyncControl_t *phControls = nullptr) override;
	FSAsyncStatus_t		AsyncDirectoryScan( const char* pSearchSpec, bool recurseFolders, void* pContext, FSAsyncScanAddFunc_t pfnAdd, FSAsyncScanCompleteFunc_t pfnDone, FSAsyncControl_t *pControl = nullptr) override;
	FSAsyncStatus_t		AsyncFinish( FSAsyncControl_t hControl, bool wait ) override;
	FSAsyncStatus_t		AsyncGetResult( FSAsyncControl_t hControl, void **ppData, int *pSize ) override;
	FSAsyncStatus_t		AsyncAbort( FSAsyncControl_t hControl ) override;
	FSAsyncStatus_t		AsyncStatus( FSAsyncControl_t hControl ) override;
	FSAsyncStatus_t		AsyncSetPriority(FSAsyncControl_t hControl, int newPriority) override;
	FSAsyncStatus_t		AsyncFlush() override;
	FSAsyncStatus_t		AsyncAppend(const char *pFileName, const void *pSrc, int nSrcBytes, bool bFreeMemory, FSAsyncControl_t *pControl) override { return AsyncWrite( pFileName, pSrc, nSrcBytes, bFreeMemory, true, pControl); }
	FSAsyncStatus_t		AsyncWrite(const char *pFileName, const void *pSrc, int nSrcBytes, bool bFreeMemory, bool bAppend, FSAsyncControl_t *pControl) override;
	FSAsyncStatus_t		AsyncWriteFile(const char *pFileName, const CUtlBuffer *pSrc, int nSrcBytes, bool bFreeMemory, bool bAppend, FSAsyncControl_t *pControl) override;
	FSAsyncStatus_t		AsyncAppendFile(const char *pDestFileName, const char *pSrcFileName, FSAsyncControl_t *pControl) override;
	void				AsyncFinishAll( int iToPriority = INT_MIN ) override;
	void				AsyncFinishAllWrites() override;
	bool				AsyncSuspend() override;
	bool				AsyncResume() override;

	void				AsyncAddRef( FSAsyncControl_t hControl ) override;
	void				AsyncRelease( FSAsyncControl_t hControl ) override;
	FSAsyncStatus_t		AsyncBeginRead( const char *pszFile, FSAsyncFile_t *phFile ) override;
	FSAsyncStatus_t		AsyncEndRead( FSAsyncFile_t hFile ) override;

	//--------------------------------------------------------
	// pack files
	//--------------------------------------------------------
	bool						AddPackFile( const char *pFileName, const char *pathID ) override;
	bool						AddPackFileFromPath( const char *pPath, const char *pakfile, bool bCheckForAppendedPack, const char *pathID );

	// converts a partial path into a full path
	// can be filtered to restrict path types and can provide info about resolved path
	const char			*RelativePathToFullPath( const char *pFileName, const char *pPathID, char *pFullPath, int fullPathBufferSize, PathTypeFilter_t pathFilter = FILTER_NONE, PathTypeQuery_t *pPathType = nullptr) override;
#if IsGameConsole()
	virtual bool				GetPackFileInfoFromRelativePath( const char *pFileName, const char *pPathID, char *pPackPath, int nPackPathBufferSize, int64 &nPosition, int64 &nLength );
#endif
	// Returns the search path, each path is separated by ;s. Returns the length of the string returned
	int					GetSearchPath( const char *pathID, bool bGetPackFiles, char *pPath, int nMaxLen ) override;
	int					GetSearchPathID( char *pPath, int nMaxLen ) override;

#if defined( TRACK_BLOCKING_IO )
	virtual void				EnableBlockingFileAccessTracking( bool state );
	virtual bool				IsBlockingFileAccessEnabled() const;
	virtual IBlockingFileItemList *RetrieveBlockingFileAccessInfo();

	virtual void				RecordBlockingFileAccess( bool synchronous, const FileBlockingItem& item );

	virtual bool				SetAllowSynchronousLogging( bool state );
#endif

	bool				GetFileTypeForFullPath( char const *pFullPath, wchar_t *buf, size_t bufSizeInBytes ) override;

	void				BeginMapAccess() override;
	void				EndMapAccess() override;
	bool				FullPathToRelativePathEx( const char *pFullpath, const char *pPathId, char *pRelative, int maxlen ) override;

	FSAsyncStatus_t				SyncRead( const FileAsyncRequest_t &request );
	FSAsyncStatus_t				SyncWrite(const char *pszFilename, const void *pSrc, int nSrcBytes, bool bFreeMemory, bool bAppend );
	FSAsyncStatus_t				SyncAppendFile(const char *pAppendToFileName, const char *pAppendFromFileName );
	FSAsyncStatus_t				SyncGetFileSize( const FileAsyncRequest_t &request );
	void						DoAsyncCallback( const FileAsyncRequest_t &request, void *pData, int nBytesRead, FSAsyncStatus_t result );

	void						SetupPreloadData() override;
	void						DiscardPreloadData() override;

	// If the "PreloadedData" hasn't been purged, then this'll try and instance the KeyValues using the fast path of compiled keyvalues loaded during startup.
	// Otherwise, it'll just fall through to the regular KeyValues loading routines
	KeyValues			*LoadKeyValues( KeyValuesPreloadType_t type, char const *filename, char const *pPathID = nullptr ) override;
	bool				LoadKeyValues( KeyValues& head, KeyValuesPreloadType_t type, char const *filename, char const *pPathID = nullptr ) override;

	DVDMode_t			GetDVDMode() override { return m_DVDMode; }
	bool				IsLaunchedFromXboxHDD() override { return m_bLaunchedFromXboxHDD; }
	bool				IsInstalledToXboxHDDCache() override { return m_bFoundXboxImageInCache; }
	bool				IsDVDHosted() override { return m_bDVDHosted; }
	bool				IsInstallAllowed() override { return m_bAllowXboxInstall; }
	bool				FixupSearchPathsAfterInstall() override;

	FSDirtyDiskReportFunc_t		GetDirtyDiskReportFunc() override { return m_DirtyDiskReportFunc; }

	void AddVPKFile( char const *pszName, SearchPathAdd_t addType = PATH_ADD_TO_TAIL ) override;
	void RemoveVPKFile( char const *pszName ) override;
	void GetVPKFileNames( CUtlVector<CUtlString> &destVector ) override;

	void				RemoveAllMapSearchPaths( void ) override;

	void						BuildExcludeList();
	void				SyncDvdDevCache() override;
	bool						FixupFATXFilename( const char *pFilename, char *pOutFilename, int nOutSize ) const;
	bool				GetStringFromKVPool( CRC32_t poolKey, unsigned int key, char *pOutBuff, int buflen ) override;

	bool				DiscoverDLC( int iController ) override;
	int					IsAnyDLCPresent( bool *pbDLCSearchPathMounted = nullptr) override;
	bool				GetAnyDLCInfo( int iDLC, unsigned int *pLicenseMask, wchar_t *pTitleBuff, int nOutTitleSize ) override;
	int					IsAnyCorruptDLC() override;
	bool				GetAnyCorruptDLCInfo( int iCorruptDLC, wchar_t *pTitleBuff, int nOutTitleSize ) override;
	bool				AddDLCSearchPaths() override;
	bool				IsSpecificDLCPresent( unsigned int nDLCPackage ) override;
	void						PrintDLCInfo() const;
	void                SetIODelayAlarm( float flTime ) override;
	bool				AddXLSPUpdateSearchPath( const void *pData, int nSize ) override;

	IIoStats			*GetIoStats() override;

public:
	//------------------------------------
	// Synchronous path for file operations
	//------------------------------------
	class CPathIDInfo
	{
	public:
		const CUtlSymbol& GetPathID() const;
		const char* GetPathIDString() const;
		void SetPathID( CUtlSymbol id );

	public:
		// See MarkPathIDByRequestOnly.
		bool m_bByRequestOnly;

	private:
		CUtlSymbol m_PathID;
		const char *m_pDebugPathID;
	};

	////////////////////////////////////////////////
	// IMPLEMENTATION DETAILS FOR CBaseFileSystem //
	////////////////////////////////////////////////

	class CSearchPath
	{
	public:
							CSearchPath( void );
							~CSearchPath( void );

		const char* GetPathString() const;
		
		// Path ID ("game", "mod", "gamebin") accessors.
		const CUtlSymbol& GetPathID() const;
		const char* GetPathIDString() const;

		// Search path (c:\hl2\hl2) accessors.
		void SetPath( CUtlSymbol id );
		const CUtlSymbol& GetPath() const;

		void SetPackFile(CPackFile *pPackFile) { m_pPackFile = pPackFile; }
		CPackFile *GetPackFile() const { return m_pPackFile; }

		int					m_storeId;

		// Used to track if its search 
		CPathIDInfo			*m_pPathIDInfo;

		bool				m_bIsDvdDevPath;

	private:
		CUtlSymbol			m_Path;
		const char			*m_pDebugPath;
		CPackFile			*m_pPackFile;
	public:
		bool				m_bIsLocalizedPath;
	};

	static void MarkLocalizedPath( CSearchPath *sp );

	class CSearchPathsVisits
	{
	public:
		void Reset()
		{
			m_Visits.RemoveAll();
		}

		bool MarkVisit( const CSearchPath &searchPath )
		{
			if ( m_Visits.Find( searchPath.m_storeId ) == m_Visits.InvalidIndex() )
			{
				MEM_ALLOC_CREDIT();
				m_Visits.AddToTail( searchPath.m_storeId );
				return false;
			}
			return true;
		}

	private:
		CUtlVector<int> m_Visits;	// This is a copy of IDs for the search paths we've visited, so 
	};

	class CSearchPathsIterator
	{
	public:
		CSearchPathsIterator( CBaseFileSystem *pFileSystem, const char **ppszFilename, const char *pszPathID, PathTypeFilter_t pathTypeFilter = FILTER_NONE )
		  : m_iCurrent( -1 ),
			m_PathTypeFilter( pathTypeFilter ),
			m_bExcluded( false )
		{
			char tempPathID[MAX_PATH];
			if ( *ppszFilename && (*ppszFilename)[0] == '/' && (*ppszFilename)[1] == '/' ) // ONLY '//' (and not '\\') for our special format
			{
				// Allow for UNC-type syntax to specify the path ID.
				pFileSystem->ParsePathID( *ppszFilename, pszPathID, tempPathID );
			}
			if ( pszPathID )
			{
				m_pathID = g_PathIDTable.AddString( pszPathID );
			}
			else
			{
				m_pathID = UTL_INVAL_SYMBOL;
			}

			if ( *ppszFilename && !Q_IsAbsolutePath( *ppszFilename ) )
			{
				// Copy paths to minimize mutex lock time
				pFileSystem->m_SearchPathsMutex.Lock();
				CopySearchPaths( pFileSystem->m_SearchPaths );
				pFileSystem->m_SearchPathsMutex.Unlock();
				V_strncpy( m_Filename, *ppszFilename, sizeof( m_Filename ) );
				V_FixSlashes( m_Filename );
			}
			else
			{
				// If it's an absolute path, it isn't worth using the paths at all. Simplify
				// client logic by pretending there's a search path of 1
				m_EmptyPathIDInfo.m_bByRequestOnly = false;
				m_EmptySearchPath.m_pPathIDInfo = &m_EmptyPathIDInfo;
				m_EmptySearchPath.SetPath( m_pathID );
				m_EmptySearchPath.m_storeId = -1;
				m_Filename[0] = '\0';
			}
		}

		CSearchPathsIterator( CBaseFileSystem *pFileSystem, const char *pszPathID, PathTypeFilter_t pathTypeFilter = FILTER_NONE )
		  : m_iCurrent( -1 ),
			m_PathTypeFilter( pathTypeFilter ),
			m_bExcluded( false )
		{
			if ( pszPathID ) 
			{
				m_pathID =  g_PathIDTable.AddString( pszPathID );
			}
			else
			{
				m_pathID =  UTL_INVAL_SYMBOL;
			}
			// Copy paths to minimize mutex lock time
			pFileSystem->m_SearchPathsMutex.Lock();
			CopySearchPaths( pFileSystem->m_SearchPaths );
			pFileSystem->m_SearchPathsMutex.Unlock();
			m_Filename[0] = '\0';
		}

		CSearchPath *GetFirst();
		CSearchPath *GetNext();

	private:
		CSearchPathsIterator( const  CSearchPathsIterator & );
		void operator=(const CSearchPathsIterator &);
		void CopySearchPaths( const CUtlVector<CSearchPath>	&searchPaths )
		{
			m_SearchPaths = searchPaths;
			for ( int i = 0; i <  m_SearchPaths.Count(); i++ )
			{
				if ( m_SearchPaths[i].GetPackFile() )
				{
					m_SearchPaths[i].GetPackFile()->AddRef();
				}
			}
		}

		int							m_iCurrent;
		CUtlSymbol					m_pathID;
		CUtlVector<CSearchPath> 	m_SearchPaths;
		CSearchPathsVisits			m_visits;
		CSearchPath					m_EmptySearchPath;
		CPathIDInfo					m_EmptyPathIDInfo;
		PathTypeFilter_t			m_PathTypeFilter;
		char						m_Filename[MAX_PATH];	// set for relative names only
		bool						m_bExcluded;
	};

	friend class CSearchPathsIterator;

	struct FindData_t
	{
		WIN32_FIND_DATA		findData;
		int					currentSearchPathID;
		CUtlVector<char>	wildCardString;
		HANDLE				findHandle;
		CSearchPathsVisits	m_VisitedSearchPaths;	// This is a copy of IDs for the search paths we've visited, so avoids searching duplicate paths.
		int					m_CurrentStoreID;		// CSearchPath::m_storeId of the current search path.
		
		CUtlSymbol			m_FilterPathID;			// What path ID are we looking at? Ignore all others. (Only set by FindFirstEx).
		
		CUtlDict<int,int>	m_VisitedFiles;			// We go through the search paths in priority order, and we use this to make sure
													// that we don't return the same file more than once.
#ifdef SUPPORT_VPK
		CUtlStringList		m_fileMatchesFromVPK;
		CUtlStringList		m_dirMatchesFromVPK;
#endif
	};

	friend class CSearchPath;

	CWhitelistHolder	m_FileWhitelist;
	int					m_WhitelistSpewFlags; // Combination of WHITELIST_SPEW_ flags.

	// logging functions
	CUtlVector< FileSystemLoggingFunc_t > m_LogFuncs;

	CThreadMutex m_SearchPathsMutex;
	CUtlVector< CSearchPath > m_SearchPaths;
	CUtlVector<CPathIDInfo*> m_PathIDInfos;
	CUtlLinkedList<FindData_t> m_FindData;

	CUtlVector<CFileHandleTimer*> m_FileHandleTimers;				// Used to debug approximate times for each file read
	CThreadFastMutex m_FileHandleTimersMutex;

	int m_iMapLoad;

	// Global list of pack file handles
	CUtlVector<CPackFile *> m_ZipFiles;

	FILE *m_pLogFile;
	bool m_bOutputDebugString;

	IThreadPool *	m_pThreadPool;

	// Statistics:
	FileSystemStatistics m_Stats;

#ifdef SUPPORT_IODELAY_MONITORING
	float m_flDelayLimit;
	float m_flLastIOTime;

	
	FORCEINLINE void NoteIO( void )
	{
		if ( m_pDelayThread )
		{
			m_flLastIOTime = Plat_FloatTime();
		}
	}
	
	CIODelayAlarmThread *m_pDelayThread;

#else
	FORCEINLINE void NoteIO( void )
	{
	}
#endif


#if defined( TRACK_BLOCKING_IO )
	CBlockingFileItemList	*m_pBlockingItems;
	bool					m_bBlockingFileAccessReportingEnabled;
	bool					m_bAllowSynchronousLogging;

	friend class			CBlockingFileItemList;
	friend class			CAutoBlockReporter;
#endif

	CFileTracker2	m_FileTracker2;

protected:
	//----------------------------------------------------------------------------
	// Purpose: Functions implementing basic file system behavior.
	//----------------------------------------------------------------------------
	virtual FILE *FS_fopen( const char *filename, const char *options, unsigned flags, int64 *size, CFileLoadInfo *pInfo ) = 0;
	virtual void FS_setbufsize( FILE *fp, unsigned nBytes ) = 0;
	virtual void FS_fclose( FILE *fp ) = 0;
	virtual void FS_fseek( FILE *fp, int64 pos, int seekType ) = 0;
	virtual long FS_ftell( FILE *fp ) = 0;
	virtual int FS_feof( FILE *fp ) = 0;
	size_t FS_fread( void *dest, size_t size, FILE *fp ) { return FS_fread( dest, (size_t)-1, size, fp ); }
	virtual size_t FS_fread( void *dest, size_t destSize, size_t size, FILE *fp ) = 0;
    virtual size_t FS_fwrite( const void *src, size_t size, FILE *fp ) = 0;
	virtual bool FS_setmode( FILE *fp, FileMode_t mode ) { return false; }
	virtual size_t FS_vfprintf( FILE *fp, const char *fmt, va_list list ) = 0;
	virtual int FS_ferror( FILE *fp ) = 0;
	virtual int FS_fflush( FILE *fp ) = 0;
	virtual char *FS_fgets( char *dest, int destSize, FILE *fp ) = 0;
	virtual int FS_stat( const char *path, struct _stat *buf ) = 0;
	virtual int FS_chmod( const char *path, int pmode ) = 0;
	virtual HANDLE FS_FindFirstFile( const char *findname, WIN32_FIND_DATA *dat) = 0;
	virtual bool FS_FindNextFile(HANDLE handle, WIN32_FIND_DATA *dat) = 0;
	virtual bool FS_FindClose(HANDLE handle) = 0;
	virtual int FS_GetSectorSize( FILE * ) { return 1; }

#if defined( TRACK_BLOCKING_IO )
	void BlockingFileAccess_EnterCriticalSection();
	void BlockingFileAccess_LeaveCriticalSection();

	CThreadMutex m_BlockingFileMutex;

#endif

	void GetFileNameForHandle( FileHandle_t handle, char *buf, size_t buflen );

protected:
	//-----------------------------------------------------------------------------
	// Purpose: For tracking unclosed files
	// NOTE:  The symbol table could take up memory that we don't want to eat here.
	// In that case, we shouldn't store them in a table, or we should store them as locally allocates stings
	//  so we can control the size
	//-----------------------------------------------------------------------------
	class COpenedFile
	{
	public:
					COpenedFile( void );
					~COpenedFile( void );

					COpenedFile( const COpenedFile& src );

		bool operator==( const COpenedFile& src ) const;

		void		SetName( char const *name );
		char const	*GetName( void ) const;

		FILE		*m_pFile;
		char		*m_pName;
	};

	//CUtlRBTree< COpenedFile, int > m_OpenedFiles;
	CThreadMutex m_OpenedFilesMutex;
	CUtlVector <COpenedFile>	m_OpenedFiles;
	CUtlStringMap< bool >		m_NonexistingFilesExtensions;
#ifdef NONEXISTING_FILES_CACHE_SUPPORT
	CUtlStringMap< double >		m_NonexistingFilesCache;
#endif

	static bool OpenedFileLessFunc( COpenedFile const& src1, COpenedFile const& src2 );

	FileWarningLevel_t			m_fwLevel;
	void						(*m_pfnWarning)( const char *fmt, ... );

	FILE						*Trace_FOpen( const char *filename, const char *options, unsigned flags, int64 *size, CFileLoadInfo *pInfo= nullptr);
	void						Trace_FClose( FILE *fp );
	void						Trace_FRead( int size, FILE* file );
	void						Trace_FWrite( int size, FILE* file );

	void						Trace_DumpUnclosedFiles( void );

public:
	void						LogAccessToFile( char const *accesstype, char const *fullpath, char const *options );
	void						FileSystemWarning( FileWarningLevel_t level, const char *fmt, ... ) const;

protected:
	// Note: if pFoundStoreID is passed in, then it will set that to the CSearchPath::m_storeId value of the search path it found the file in.
	const char*					FindFirstHelper( const char *pWildCard, const char *pPathID, FileFindHandle_t *pHandle, int *pFoundStoreID );
	bool						FindNextFileHelper( FindData_t *pFindData, int *pFoundStoreID );

	void 						FindFileAbsoluteListHelper( CUtlVector< CUtlString > &outAbsolutePathNames, FindData_t &findData, const char *pAbsoluteFindName );

	void						AddMapPackFile( const char *pPath, const char *pPathID, SearchPathAdd_t addType );
	void						AddPackFiles( const char *pPath, const CUtlSymbol &pathID, SearchPathAdd_t addType, int iForceInsertIndex = 0 );
	bool						PreparePackFile( CPackFile &packfile, int offsetofpackinmetafile, int64 filelen );

	// Goes through all the search paths (or just the one specified) and calls FindFile on them. Returns the first successful result, if any.
	FileHandle_t				FindFileInSearchPaths( const char *pFileName, const char *pOptions, const char *pathID, unsigned flags, char **ppszResolvedFilename = nullptr, bool bTrackCRCs=false );

	bool						HandleOpenFromZipFile( CFileOpenInfo &openInfo );
	void		 				HandleOpenFromPackFile( CPackFile *pPackFile, CFileOpenInfo &openInfo ) const;
	void						HandleOpenRegularFile( CFileOpenInfo &openInfo, bool bIsAbsolutePath );

	FileHandle_t				FindFile( const CSearchPath *path, const char *pFileName, const char *pOptions, unsigned flags, char **ppszResolvedFilename = nullptr, bool bTrackCRCs=false );
	int							FastFindFile( const CSearchPath *path, const char *pFileName );
	long						FastFileTime( const CSearchPath *path, const char *pFileName );

	const char					*GetWritePath( const char *pFilename, const char *pathID );

	// Computes a full write path
	void						ComputeFullWritePath( char* pDest, int maxlen, const char *pWritePathID, char const *pRelativePath );

	void						AddSearchPathInternal( const char *pPath, const char *pathID, SearchPathAdd_t addType, bool bAddPackFiles, int iForceInsertIndex = 0 );

	// Opens a file for read or write
	FileHandle_t OpenForRead( const char *pFileName, const char *pOptions, unsigned flags, const char *pathID, char **ppszResolvedFilename = nullptr);
	FileHandle_t OpenForWrite( const char *pFileName, const char *pOptions, const char *pathID );
	CSearchPath *FindWritePath( const char *pFilename, const char *pathID );

	// Helper function for fs_log file logging
	void LogFileAccess( const char *pFullFileName ) const;
#if IsPlatformPS3()
	virtual bool PrefetchFile( const char *pFileName, int nPriority, bool bPersist );
	virtual bool PrefetchFile( const char *pFileName, int nPriority, bool bPersist, int64 nOffset, int64 nSize );
	virtual void FlushCache();
	virtual void SuspendPrefetches( const char *pWhy );
	virtual void ResumePrefetches( const char *pWhy );
	virtual void OnSaveStateChanged( bool bSaving );
	virtual bool IsPrefetchingDone();
#endif

	bool LookupKeyValuesRootKeyName( char const *filename, char const *pPathID, char *rootName, size_t bufsize );

	// If bByRequestOnly is -1, then it will default to false if it doesn't already exist, and it 
	// won't change it if it does already exist. Otherwise, it will be set to the value of bByRequestOnly.
	CPathIDInfo*				FindOrAddPathIDInfo( const CUtlSymbol &id, int bByRequestOnly );
	static bool					FilterByPathID( const CSearchPath *pSearchPath, const CUtlSymbol &pathID );

	// Global/shared filename/path table
	CUtlFilenameSymbolTable		m_FileNames;

	// This manages most of the info we use for pure servers (whether files came from Steam caches or off-disk, their CRCs, which ones are unverified, etc).
	CFileTracker	m_FileTracker;
	int				m_WhitelistFileTrackingEnabled;	// -1 if unset, 0 if disabled (single player), 1 if enabled (multiplayer).
	FSDirtyDiskReportFunc_t m_DirtyDiskReportFunc;

	static CUtlSymbol			m_GamePathID;
	static CUtlSymbol			m_BSPPathID;

	static DVDMode_t			m_DVDMode;
	static bool					m_bFoundXboxImageInCache;
	static bool					m_bLaunchedFromXboxHDD;
	static bool					m_bDVDHosted;
	static bool					m_bAllowXboxInstall;
	static bool					m_bSearchPathsPatchedAfterInstall;

	// Pack exclude paths are strictly for 360 to allow holes in search paths and pack files
	// which fall through to support new or dynamic data on the host pc.
	struct ExcludeFilePath_t
	{
		FileNameHandle_t m_hName;
		FileNameHandle_t m_hFixedName;
	};
	static CUtlVector< FileNameHandle_t > m_ExcludeFilePaths;
#ifdef SUPPORT_VPK
	CUtlVector< CPackedStore *> m_VPKFiles;
	CUtlDict<int,int> m_VPKDirectories;
	CPackedStoreFileHandle FindFileInVPKs( const char *pFileName );
#endif

	static CUtlSortVector< DLCContent_t, CDLCLess > m_DLCContents;
	static CUtlVector< DLCCorrupt_t > m_CorruptDLC;
};

inline const CUtlSymbol& CBaseFileSystem::CPathIDInfo::GetPathID() const
{
	return m_PathID;
}


inline const char* CBaseFileSystem::CPathIDInfo::GetPathIDString() const
{
	return g_PathIDTable.String( m_PathID );
}


inline const char* CBaseFileSystem::CSearchPath::GetPathString() const
{
	return g_PathIDTable.String( m_Path );
}


inline void CBaseFileSystem::CPathIDInfo::SetPathID( CUtlSymbol sym )
{
	m_PathID = sym;
	m_pDebugPathID = GetPathIDString();
}


inline const CUtlSymbol& CBaseFileSystem::CSearchPath::GetPathID() const
{
	return m_pPathIDInfo->GetPathID();
}


inline const char* CBaseFileSystem::CSearchPath::GetPathIDString() const
{
	return m_pPathIDInfo->GetPathIDString();
}


inline void CBaseFileSystem::CSearchPath::SetPath( CUtlSymbol id )
{
	m_Path = id;
	m_pDebugPath = g_PathIDTable.String( m_Path );
	MarkLocalizedPath( this );
}


inline const CUtlSymbol& CBaseFileSystem::CSearchPath::GetPath() const
{
	return m_Path;
}


inline bool CBaseFileSystem::FilterByPathID( const CSearchPath *pSearchPath, const CUtlSymbol &pathID )
{
	if ( (UtlSymId_t)pathID == UTL_INVAL_SYMBOL )
	{
		// They didn't specify a specific search path, so if this search path's path ID is by
		// request only, then ignore it.
		return pSearchPath->m_pPathIDInfo->m_bByRequestOnly;
	}

	// Bit of a hack, but specifying "BSP" as the search path will search in "GAME" for only the map/.bsp pack file path
	if ( pathID != m_BSPPathID )
		return (pSearchPath->GetPathID() != pathID);

	if ( pSearchPath->GetPathID() != m_GamePathID )
		return true;

	if ( !pSearchPath->GetPackFile() )
		return true;

	if ( !pSearchPath->GetPackFile()->m_bIsMapPath )
		return true;

	return false;
}


// Pack file handle implementation:
                 
inline CPackFileHandle::CPackFileHandle( CPackFile* pOwner, int64 nBase, unsigned int nLength, unsigned int nIndex, unsigned int nFilePointer )
{
	m_pOwner = pOwner;
	m_nBase = nBase;
	m_nLength = nLength;
	m_nIndex = nIndex;
	m_nFilePointer = nFilePointer;
	pOwner->AddRef();
}

inline CPackFileHandle::~CPackFileHandle()
{
	m_pOwner->m_mutex.Lock();
	--m_pOwner->m_nOpenFiles;
	if ( m_pOwner->m_nOpenFiles == 0 && m_pOwner->m_bIsMapPath )
	{
		if ( m_pOwner->m_hPackFileHandleFS )
		{
			m_pOwner->m_fs->Trace_FClose( m_pOwner->m_hPackFileHandleFS );
			m_pOwner->m_hPackFileHandleFS = nullptr;
		}
	}
	m_pOwner->Release();
	m_pOwner->m_mutex.Unlock();
}

inline void CPackFileHandle::SetBufferSize( int nBytes ) const
{
	if ( m_pOwner->m_hPackFileHandleFS )
	{
		m_pOwner->m_fs->FS_setbufsize( m_pOwner->m_hPackFileHandleFS, nBytes );
	}
}

inline int CPackFileHandle::GetSectorSize() const
{ 
	return m_pOwner->GetSectorSize(); 
}

inline int64 CPackFileHandle::AbsoluteBaseOffset() const
{ 
	return m_pOwner->GetPackFileBaseOffset() + m_nBase;
}

// Pack file implementation:
inline CPackFile::CPackFile()
{
	m_FileLength = 0;
	m_hPackFileHandleFS = nullptr;
	m_fs = nullptr;
	m_nBaseOffset = 0;
	m_bIsMapPath = false;
	m_lPackFileTime = 0L;
	m_refCount = 0;
	m_nOpenFiles = 0;
	m_PackFileID = 0;
}

inline CPackFile::~CPackFile()
{
	if ( m_nOpenFiles )
	{
		Error( "Closing pack file with %d open files!\n", m_nOpenFiles );
	}

	if ( m_hPackFileHandleFS )
	{
		m_fs->FS_fclose( m_hPackFileHandleFS );
		m_hPackFileHandleFS = nullptr;
	}

	m_fs->m_ZipFiles.FindAndRemove( this );
}


inline int CPackFile::GetSectorSize() const
{
	if ( m_hPackFileHandleFS )
	{
		return m_fs->FS_GetSectorSize( m_hPackFileHandleFS );
	}
#ifdef SUPPORT_VPK
	else if ( m_hPackFileHandleVPK )
	{
		return 2048;
	}
#endif
	else
	{
		return -1;
	}
}


#if defined( TRACK_BLOCKING_IO )

class CAutoBlockReporter
{
public:

	CAutoBlockReporter( CBaseFileSystem *fs, bool synchronous, char const *filename, int eBlockType, int nTypeOfAccess ) :
		m_pFS( fs ),
		m_Item( eBlockType, filename, 0.0f, nTypeOfAccess ),
		m_bSynchronous( synchronous )
	{
		Assert( m_pFS );
		m_Timer.Start();
	}
	
	CAutoBlockReporter( CBaseFileSystem *fs, bool synchronous, FileHandle_t handle, int eBlockType, int nTypeOfAccess ) :
		m_pFS( fs ),
		m_Item( eBlockType, NULL, 0.0f, nTypeOfAccess ),
		m_bSynchronous( synchronous )
	{
		Assert( m_pFS );
		char name[ 512 ];
		m_pFS->GetFileNameForHandle( handle, name, sizeof( name ) );
		m_Item.SetFileName( name );
		m_Timer.Start();
	}

	~CAutoBlockReporter()
	{
		m_Timer.End();
		m_Item.m_flElapsed = m_Timer.GetDuration().GetSeconds();
		m_pFS->RecordBlockingFileAccess( m_bSynchronous, m_Item );
	}

private:

	CBaseFileSystem		*m_pFS;

	CFastTimer			m_Timer;
	FileBlockingItem	m_Item;
	bool				m_bSynchronous;
};

#define AUTOBLOCKREPORTER_FN( name, fs, sync, filename, blockType, accessType )		CAutoBlockReporter block##name( fs, sync, filename, blockType, accessType );
#define AUTOBLOCKREPORTER_FH( name, fs, sync, handle, blockType, accessType )		CAutoBlockReporter block##name( fs, sync, handle, blockType, accessType );

#else

#define AUTOBLOCKREPORTER_FN( name, fs, sync, filename, blockType, accessType )	// Nothing
#define AUTOBLOCKREPORTER_FH( name, fs, sync, handle , blockType, accessType )	// Nothing

#endif

// singleton accessor
CBaseFileSystem *BaseFileSystem();

#include "tier0/memdbgoff.h"

#endif // BASEFILESYSTEM_H
