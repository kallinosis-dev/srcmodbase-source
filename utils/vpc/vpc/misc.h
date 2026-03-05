#pragma once
// See https://xkcd.com/1077/

extern const char* g_pOption_ImportLibrary;
extern const char* g_pOption_OutputFile;
extern const char* g_pOption_AdditionalDependencies;
extern const char* g_pOption_OrderOnlyFileDependencies;
extern const char* g_pOption_OrderOnlyProjectDependencies;
extern const char* g_pOption_AdditionalDependencies_Proj;
extern const char* g_pOption_AdditionalIncludeDirectories;
extern const char* g_pOption_AdditionalProjectDependencies;
extern const char* g_pOption_AdditionalOutputFiles;
extern const char* g_pOption_PreprocessorDefinitions;
extern const char* g_pOption_PrecompiledHeader;
extern const char* g_pOption_UsePCHThroughFile;
extern const char* g_pOption_PrecompiledHeaderFile;
extern const char* g_pOption_ForceInclude;
extern const char* g_pOption_ExcludedFromBuild;
extern const char* g_pOption_CommandLine;
extern const char* g_pOption_ConfigurationType;
extern const char* g_pOption_Description;
extern const char* g_pOption_GCC_ExtraCompilerFlags;
extern const char* g_pOption_GCC_ExtraCxxCompilerFlags;
extern const char* g_pOption_GCC_ExtraLinkerFlags;
extern const char* g_pOption_POSIX_RPaths;
extern const char* g_pOption_GameOutputFile;
extern const char* g_pOption_OptimizerLevel;
extern const char* g_pOption_Outputs;
extern const char* g_pOption_PotentialOutputs;
extern const char* g_pOption_PostBuildEvent;
extern const char* g_pOption_SymbolVisibility;
extern const char* g_pOption_SystemLibraries;
extern const char* g_pOption_BuildMultiArch;
extern const char* g_pOption_TreatWarningsAsErrors;
extern const char* g_pOption_DisableLinkerDeadCodeElimination;

bool VPC_IsPlatformWindows(const char* pPlatformName);
bool VPC_IsPlatformLinux(const char* pPlatformName);
bool VPC_IsPlatformAndroid(const char* pPlatformName);
bool VPC_IsPlatform32Bits(const char* pPlatformName);
bool VPC_IsPlatform64Bits(const char* pPlatformName);


bool CharStrEq(const char* pStr, char ch);
