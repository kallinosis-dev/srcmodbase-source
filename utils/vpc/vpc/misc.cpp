#include "misc.h"

#include "tier1/strtools.h"

// Stuff that we might encounter in a vpc file that parts of vpc care about
const char* g_pOption_ImportLibrary = "$ImportLibrary";
const char* g_pOption_OutputFile = "$OutputFile";
const char* g_pOption_AdditionalDependencies = "$AdditionalDependencies";
const char* g_pOption_OrderOnlyFileDependencies = "$OrderOnlyFileDependencies";
const char* g_pOption_OrderOnlyProjectDependencies = "$OrderOnlyProjectDependencies";
const char* g_pOption_AdditionalDependencies_Proj = "$AdditionalDependencies_Proj";
const char* g_pOption_AdditionalIncludeDirectories = "$AdditionalIncludeDirectories";
const char* g_pOption_AdditionalProjectDependencies = "$AdditionalProjectDependencies";
const char* g_pOption_AdditionalOutputFiles = "$AdditionalOutputFiles";
const char* g_pOption_PreprocessorDefinitions = "$PreprocessorDefinitions";
const char* g_pOption_PrecompiledHeader = "$Create/UsePrecompiledHeader";
const char* g_pOption_UsePCHThroughFile = "$Create/UsePCHThroughFile";
const char* g_pOption_PrecompiledHeaderFile = "$PrecompiledHeaderFile";
const char* g_pOption_ForceInclude = "$ForceIncludes";
const char* g_pOption_ExcludedFromBuild = "$ExcludedFromBuild";
const char* g_pOption_CommandLine = "$CommandLine";
const char* g_pOption_ConfigurationType = "$ConfigurationType";
const char* g_pOption_Description = "$Description";
const char* g_pOption_GCC_ExtraCompilerFlags = "$GCC_ExtraCompilerFlags";
const char* g_pOption_GCC_ExtraCxxCompilerFlags = "$GCC_ExtraCxxCompilerFlags";
const char* g_pOption_GCC_ExtraLinkerFlags = "$GCC_ExtraLinkerFlags";
const char* g_pOption_POSIX_RPaths = "$POSIX_RPaths";
const char* g_pOption_GameOutputFile = "$GameOutputFile";
const char* g_pOption_OptimizerLevel = "$OptimizerLevel";
const char* g_pOption_Outputs = "$Outputs";
const char* g_pOption_PotentialOutputs = "$PotentialOutputs";
//outputs that don't contribute to timestamp triggers, but might be output by the tool (Makefiles needs to know their timestamp *might* change)
const char* g_pOption_PostBuildEvent = "$PostBuildEvent";
const char* g_pOption_SymbolVisibility = "$SymbolVisibility";
const char* g_pOption_SystemLibraries = "$SystemLibraries";
const char* g_pOption_BuildMultiArch = "$BuildMultiArch";
const char* g_pOption_TreatWarningsAsErrors = "$TreatWarningsAsErrors";
const char* g_pOption_DisableLinkerDeadCodeElimination = "$DisableLinkerDeadCodeElimination";



bool VPC_IsPlatformWindows(const char* pPlatformName)
{
	return !V_stricmp_fast(pPlatformName, "WIN32") ||
		!V_stricmp_fast(pPlatformName, "WIN64");
}

bool VPC_IsPlatformLinux(const char* pPlatformName)
{
	return !V_stricmp_fast(pPlatformName, "LINUX64") ||
		!V_stricmp_fast(pPlatformName, "LINUX32") ||
		!V_stricmp_fast(pPlatformName, "LINUXSERVER64") ||
		!V_stricmp_fast(pPlatformName, "LINUXSTEAMRTARM32HF") ||
		!V_stricmp_fast(pPlatformName, "LINUXSTEAMRTARM64HF");
}

bool VPC_IsPlatformAndroid(const char* pPlatformName)
{
	return !V_stricmp_fast(pPlatformName, "ANDROIDARM32") ||
		!V_stricmp_fast(pPlatformName, "ANDROIDARM64") ||
		!V_stricmp_fast(pPlatformName, "ANDROIDMIPS32") ||
		!V_stricmp_fast(pPlatformName, "ANDROIDMIPS64") ||
		!V_stricmp_fast(pPlatformName, "ANDROIDX8632") ||
		!V_stricmp_fast(pPlatformName, "ANDROIDX8664");
}

bool VPC_IsPlatform32Bits(const char* pPlatformName)
{
	return !V_stricmp_fast(pPlatformName, "ANDROIDARM32") ||
		!V_stricmp_fast(pPlatformName, "ANDROIDMIPS32") ||
		!V_stricmp_fast(pPlatformName, "ANDROIDX8632") ||
		!V_stricmp_fast(pPlatformName, "LINUXSTEAMRTARM32HF") ||
		!V_stricmp_fast(pPlatformName, "LINUX32") ||
		!V_stricmp_fast(pPlatformName, "WIN32");
}

bool VPC_IsPlatform64Bits(const char* pPlatformName)
{
	return !V_stricmp_fast(pPlatformName, "ANDROIDARM64") ||
		!V_stricmp_fast(pPlatformName, "ANDROIDMIPS64") ||
		!V_stricmp_fast(pPlatformName, "ANDROIDX8664") ||
		!V_stricmp_fast(pPlatformName, "LINUX64") ||
		!V_stricmp_fast(pPlatformName, "LINUXSERVER64") ||
		!V_stricmp_fast(pPlatformName, "LINUXSTEAMRTARM64HF") ||
		!V_stricmp_fast(pPlatformName, "WIN64");
}

bool CharStrEq(const char* pStr, char ch)
{
	return pStr[0] == ch && pStr[1] == 0;
}
