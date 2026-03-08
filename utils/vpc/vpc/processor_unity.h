//========= Copyright Valve Corporation, All rights reserved. =========================//
//
// Purpose: VPC (unity file support)
//
// Summary:
//  - to enable unity files for a project, change $Project to $UnityProject in the *LEAF* VPC file
//  - each $Folder replaced with $UnityFolder in a VPC file will generate one or more unity files
//   o this will include only files listed in that folder, *NOT* child folders
//   o each unity file includes about 20 CPP files (to balance serial compile speed with
//     parallelism) and a unity file including just one source file will not be generated
//   o files are grouped based on compiler settings, so for example files using different
//     PCHs will be segregated into different unity files
//  - source files are #included in unity files unless locally modified:
//   o read-only files are included in their unity files and excluded from the normal build
//   o writeable (locally-edited) files are excluded from unity files and built normally
//     (this minimizes rebuild time AND ensures that edited files will build independently
//     of their containing unity file, i.e so you don't forget to add new header includes!)
//  - schema/qt/protobuf generated files get special handling:
//   o the unity file includes the generated (*.gen_cpp/moc_*.cpp) file, not the source file
//   o the unity file may include different files for debug/release
//  - the ValveVSAddin (Visual Studio addin) provides supplementary functionality
//   o it detects changes to the readonly status of files at build-time and calls into
//     VPC (using '/unity_update') to update the unity files as appropriate, so you
//     needn't re-VPC your project whenever you check a file out
//   o it provides 'smart' compile commands which compile the appropriate 'output' file
//     for the current file; for example, running 'compile' on a schematized header will
//     compile the generated (debug/release) gen_cpp file, or its containing unity file
//   o VPC writes out a manifest file to provide the file mapping data to the addin
//     (see VPC_GeneratedFiles_OnParseProjectEnd)
//
//=====================================================================================//

#pragma once

class CProjectFile;
class CVCProjGenerator;

extern void					VPC_Unity_OnParseProjectStart( void );
extern void					VPC_Unity_OnParseProjectEnd( CVCProjGenerator *pDataCollector );
extern bool					VPC_Unity_UpdateUnityFiles(char const* const* ppArgs, int nArgs);
extern CProjectFile *		VPC_Unity_GetContainingUnityFile( CProjectFile *pInputFile, const char *pConfigName, CVCProjGenerator *pDataCollector );
