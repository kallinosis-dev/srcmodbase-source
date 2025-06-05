#include "projectcache.h"

#include "ibaseprojectgenerator.h"
#include "tier1/utlstring.h"
#include "tier1/fmtstr.h"
#include "tier1/utlbuffer.h"
#include "tier1/keyvalues.h"
#include "sys_utils.h"
#include "vpc.h"

#define VPC_CACHE_EXTENSION			"vpc_cache"
#define VPC_CACHE_VERSION			1



void ProjectCache::SetTargetPlatform(char const* targetPlatform)
{
	Assert(targetPlatform && targetPlatform[0]);
	_targetPlatform = targetPlatform;
}

void ProjectCache::SetSupplementalString(char const* supplementalString)
{
	_supplementalString = supplementalString;
}

void ProjectCache::LoadVPCCache(const char* szScriptFileName, KeyValues& intoKV)
{
	CFmtStr cacheFileName("%s." VPC_CACHE_EXTENSION, szScriptFileName);
	intoKV.Clear();

	CUtlBuffer kvBuffer;
	kvBuffer.SetBufferType(true, false);
	if (!Sys_LoadFileIntoBuffer(cacheFileName.Get(), kvBuffer, true))
		return;

	//try to load existing cache to preserve other target platforms. Not a huge deal if it fails though
	if (!intoKV.LoadFromBuffer(cacheFileName.Get(), kvBuffer))
		return;

	if (intoKV.GetInt("CacheVersion", 0) == VPC_CACHE_VERSION)
		return;

	intoKV.Clear();
	intoKV.SetInt("CacheVersion", VPC_CACHE_VERSION);
}

void ProjectCache::SaveVPCCache(const char* szScriptFileName, KeyValues& cacheKV)
{
	CFmtStr cacheFileName("%s." VPC_CACHE_EXTENSION, szScriptFileName);

	CUtlBuffer kvBuffer;
	kvBuffer.SetBufferType(true, false);

	cacheKV.RecursiveSaveToFile(kvBuffer, 0);
	Sys_WriteFileIfChanged(cacheFileName.Get(), kvBuffer, true);
}


//-----------------------------------------------------------------------------
// Operates quietly, caller decides fate of informative status.
//-----------------------------------------------------------------------------
bool ProjectCache::IsProjectCurrent(const char* szScriptFileName, CUtlString& projectStatusString) const
{
	KeyValues kvCache("vpc_cache");
	LoadVPCCache(szScriptFileName, kvCache);

	KeyValues* pKVTargetKey = kvCache.FindKey(_targetPlatform);
	if (!pKVTargetKey)
		return false;

	//generally the primary vcxproj/makefile
	const char* szCRCFile = pKVTargetKey->GetString("CRCFile");

	if (!szCRCFile || !Sys_Exists(szCRCFile))
	{
		return false;
	}

	// check output files for trivial existence
	{
		KeyValues* pKVOutputs = pKVTargetKey->FindKey("OutputFiles");
		if (pKVOutputs)
		{
			for (KeyValues* pKVIter = pKVOutputs->GetFirstSubKey(); pKVIter; pKVIter = pKVIter->GetNextKey())
			{
				const char* szOutput = pKVIter->GetString();
				if (szOutput && szOutput[0] && !Sys_Exists(szOutput))
				{
					return false;
				}
			}
		}
	}

	char errorString[1024];
	errorString[0] = '\0';
	bool bCRCValid = VPC_CheckProjectDependencyCRCs(szCRCFile, _supplementalString, errorString,
		sizeof(errorString));

	projectStatusString = errorString;

	return bCRCValid;
}

void ProjectCache::MakeStatusString(CUtlString& statusString, char const* szScriptFileName, bool crcPassed)
{
	if(!g_pVPC->IsVerbose())
	{
		// The detailed CRC error/results string is undesired, it doesn't matter why the CRC failed/succeeded.
		// By popular request, verbosity is used as the enabler, when the CRC yields unexpected results.
		statusString.Clear();
	}
	else if(!statusString.IsEmpty()) // process the detailed informational CRC status
	{
		// error string has varying contents, no expectation on CR/LF, ensure appended status appears exactly contiguous on next line
		statusString.TrimRight();
		statusString += "\n";
	}


	// The project status string is a terse summary.
	// This is a utility call in varying contexts. The caller decides whether to echo.
	if (crcPassed)
	{
		statusString += CFmtStrMax("Valid: '%s' Passes CRC Checks.", szScriptFileName);
	}
	else
	{
		statusString += CFmtStrMax("Stale: '%s' Requires Rebuild.", szScriptFileName);
	}
}

void ProjectCache::UpdateCacheFile(const char* szScriptFileName, IBaseProjectGenerator* projectGenerator) const
{
	KeyValues kvCache("vpc_cache");

	//try to load existing cache to preserve other target platforms. Not a huge deal if it fails though
	LoadVPCCache(szScriptFileName, kvCache);

	Assert(_targetPlatform);
	KeyValues* pKVTargetKey = kvCache.FindKey(_targetPlatform, true);

	//nuke the old values for this target platform
	pKVTargetKey->Clear();

	const char* szOutputFileName = projectGenerator->GetOutputFileName();
	Assert(szOutputFileName);

	pKVTargetKey->SetString("CRCFile", CFmtStr("%s." VPCCRCCHECK_FILE_EXTENSION, szOutputFileName));

	{
		KeyValues* pKVOutputs = pKVTargetKey->CreateKey("OutputFiles");

		int nKeyName = 0;
		pKVOutputs->SetString(CFmtStr("%d", nKeyName++).Get(), szOutputFileName);

		//ancillary files
		//TODO: The project generator should probably control this
		if (g_pVPC->conditionals.IsDefined("GENERATE_MAKEFILE_VCXPROJ"))
		{
			// IsProjectCurrent is only called once even though
			// we're generating twice and thus have more files to check,
			// so we special-case here to make sure all files of
			// interest are checked.
			CUtlPathStringHolder extraOutFile(szOutputFileName, ".vcxproj");
			pKVOutputs->SetString(CFmtStr("%d", nKeyName++).Get(), extraOutFile.Get());
			extraOutFile.Append(".filters");
			pKVOutputs->SetString(CFmtStr("%d", nKeyName++).Get(), extraOutFile.Get());
		}
		else
		{
			CUtlPathStringHolder extraOutFile(szOutputFileName, ".filters");
			pKVOutputs->SetString(CFmtStr("%d", nKeyName++).Get(), extraOutFile.Get());
		}
	}

	SaveVPCCache(szScriptFileName, kvCache);
}
