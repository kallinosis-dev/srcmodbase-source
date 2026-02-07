#pragma once
#include "generatordefinition.h"
#include "scriptsource.h"
#include "tier1/checksum_crc.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"

class IBaseProjectGenerator;

typedef int scriptIndex_t;
struct script_t
{
	CUtlString		name;
	CUtlString		m_condition;
};

struct scriptList_t
{
	scriptList_t()
	{
		m_crc = 0;
	}

	CUtlString	m_scriptName;
	CRC32_t		m_crc;
	bool		m_bCRCCheck;
};

enum VpcFileFlags_t
{
	VPC_FILE_FLAGS_NONE = 0,
	VPC_FILE_FLAGS_SCHEMA = 1 << 0,
	VPC_FILE_FLAGS_DYNAMIC = 1 << 1,
	VPC_FILE_FLAGS_QT = 1 << 2,
	VPC_FILE_FLAGS_CREATE_PCH = 1 << 3,
	VPC_FILE_FLAGS_NO_PCH = 1 << 4,
	VPC_FILE_FLAGS_SCHEMA_INCLUDE = 1 << 5,
	VPC_FILE_FLAGS_STATIC_LIB = 1 << 6,
	VPC_FILE_FLAGS_IMPORT_LIB = 1 << 7,
	VPC_FILE_FLAGS_SHARED_LIB = 1 << 8,
};

enum VpcFolderFlags_t
{
	VPC_FOLDER_FLAGS_NONE = 0,
	VPC_FOLDER_FLAGS_DYNAMIC = 1 << 0,
	VPC_FOLDER_FLAGS_UNITY = 1 << 1
};

enum MacroType_t { VPC_MACRO_VALUE, VPC_MACRO_EMPTY_STRING };

enum MacroRequiredType_t { VPC_MACRO_REQUIRED_NOT_EMPTY, VPC_MACRO_REQUIRED_ALLOW_EMPTY };


class CProjectScriptParser
{
	CScript _script;
	IBaseProjectGenerator* _projgen;

	bool _quiet = false;

public:
	CProjectScriptParser(IBaseProjectGenerator* projgen) : _projgen(projgen)
	{
	}

	void SetQuiet(bool quiet) { _quiet = quiet; }
	bool GetQuiet() const { return _quiet; }

	void Parse(char const* scriptName, int depth);

private:
	using NameTranslator = void (*)(CUtlStringBuilder* pStrBuf);

	void Keyword_FileBuildOrderModifier();
	void ParseFileSection();
	void Keyword_AddFilesByPattern();
	void ParseFileList(CUtlVector<CUtlString>& files, NameTranslator nameTranslator = nullptr);
	void Keyword_AddFile(VpcFileFlags_t iFileFlags, NameTranslator nameTranslator);
	void Keyword_ImportLibrary(bool bRemove, bool bExternal);
	void Keyword_LinkerLibrary(bool bRemove, bool bExternal);
	void Keyword_SharedLibrary(bool bRemove);
	void AddLibraryDependencies(const char* pLibPath);
	void LibDepends(char const* pDefaultPath, char const* pFileNamePrefix, char const* pSuffix);
	void Keyword_LibDependsOnLib();
	void Keyword_LibDependsOnImpLib();
	void Keyword_RemoveFile(NameTranslator nameTranslator = nullptr);
	void Keyword_Shaders(int depth);
	void Keyword_Folder(VpcFolderFlags_t iFolderFlags = VPC_FOLDER_FLAGS_NONE);
	void Keyword_Macro(::MacroType_t eMacroType);
	void Keyword_MacroRequired(MacroRequiredType_t eMacroRequiredType);
	void Keyword_LoadAddressMacro();
	void Keyword_LoadAddressMacroAlias();
	void LoadAddressMacroAuto(bool bPad);
	void Keyword_LoadAddressMacroAuto();
	void Keyword_LoadAddressMacroAuto_Padded();
	void Keyword_Conditional(bool bOverrideReserved);
	void Keyword_IgnoreRedundancyWarning();
	void Keyword_Linux();
	void PrepareToReadScript(const char* pInputScriptName, int depth, char*& pScriptBuffer,
	                         CUtlString* pFixedScriptName);
	void HandleIncludeStatement(int depth);
	void HandleProjectCommands(int depth);
	void Keyword_Project(int depth);
	void Config_Macro();
	void Config_Keyword(configKeyword_e keyword, const char* pkeywordToken);
	void Keyword_Configuration();
	void Keyword_FileConfiguration();
	static bool IsBuiltInFileType(const char* pExtension);
	void Keyword_CustomBuildStep();
	void Keyword_CustomAutoScript();
	void ParseProjectScriptParameters(const char* szScriptName, int depth);
};