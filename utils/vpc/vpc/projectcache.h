#pragma once

class IBaseProjectGenerator;
class CUtlString;
class KeyValues;

class ProjectCache
{
	char const* _targetPlatform = nullptr;
	char const* _supplementalString = nullptr;

public:
	void SetTargetPlatform(char const* targetPlatform);
	void SetSupplementalString(char const* supplementalString);

	bool		IsProjectCurrent(const char* szScriptFileName, CUtlString& projectStatusString) const;

	static void		MakeStatusString(CUtlString& statusString, char const* szScriptFileName, bool crcPassed);

	void		UpdateCacheFile(const char* szScriptFileName, IBaseProjectGenerator* projectGenerator) const;

private:
	static void		LoadVPCCache(const char* szScriptFileName, KeyValues& intoKV);
	static void		SaveVPCCache(const char* szScriptFileName, KeyValues& cacheKV);

};
