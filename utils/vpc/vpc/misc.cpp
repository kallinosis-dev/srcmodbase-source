#include "misc.h"

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
const char* g_pOption_LocalFrameworks = "$LocalFrameworks";
const char* g_pOption_OptimizerLevel = "$OptimizerLevel";
const char* g_pOption_Outputs = "$Outputs";
const char* g_pOption_PotentialOutputs = "$PotentialOutputs";
//outputs that don't contribute to timestamp triggers, but might be output by the tool (Makefiles needs to know their timestamp *might* change)
const char* g_pOption_PostBuildEvent = "$PostBuildEvent";
const char* g_pOption_SymbolVisibility = "$SymbolVisibility";
const char* g_pOption_SystemFrameworks = "$SystemFrameworks";
const char* g_pOption_SystemLibraries = "$SystemLibraries";
const char* g_pOption_BuildMultiArch = "$BuildMultiArch";
const char* g_pOption_TreatWarningsAsErrors = "$TreatWarningsAsErrors";
const char* g_pOption_DisableLinkerDeadCodeElimination = "$DisableLinkerDeadCodeElimination";