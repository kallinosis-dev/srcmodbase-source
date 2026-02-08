//========= Copyright 1996-2016, Valve Corporation, All rights reserved. ==============//
//
//
//=====================================================================================//

#include "tier1/fmtstr.h"
#include "tier1/splitstring.h"
#include "tier1/utlvector.h"

#include "macros.h"
#include "misc.h"
#include "projectgenerator_vcproj.h"
#include "scriptutil.h"
#include "vpc.h"

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void CleanStrings( CUtlVector< CUtlString > &strings )
{
	for ( int i = 0; i < strings.Count(); i++ )
	{
		// Remove surrounding whitespace & quotes
		// NOTE: strings[i].Length() is temporarily invalidated
		char *pString = strings[i].Access();
		V_StripLeadingWhitespace(  pString );
		V_StripTrailingWhitespace( pString );
		V_StripSurroundingQuotes(  pString );
		strings[i].Set( pString );
	}
}


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
const char *VPC_ResolveCompilerMacrosInString( const char *szSourceString, CUtlString &outputScratchSpace, CProjectConfiguration *pRootConfig, CProjectConfiguration *pFileConfig )
{
	//this should probably be driven from the .def file, but that's a lot more work than I want to sign up for at the moment
	
	CUtlString tempString;
	CUtlString propertyString;

	auto replaceWithString = 
		[&] ( const char *pFind, const char *pReplaceWith ) -> bool
		{
			if ( V_strstr( szSourceString, pFind ) != nullptr )
			{
				propertyString.Clear();
				tempString = szSourceString;
				outputScratchSpace = tempString.Replace( pFind, pReplaceWith, true );
				szSourceString = outputScratchSpace.Get();
				return true;
			}
			return false;
		};

	auto replaceWithProperty = 
		[&] ( const char *pFind, configKeyword_e replaceKeyword, const char *szReplaceVPCProperty ) -> bool
		{
			if ( V_strstr( szSourceString, pFind ) != nullptr )
			{
				propertyString.Clear();
				VPC_GetPropertyString( replaceKeyword, pRootConfig, pFileConfig, szReplaceVPCProperty, &propertyString );
				if ( !propertyString.IsEmpty() )
				{
					tempString = szSourceString;
					outputScratchSpace = tempString.Replace( pFind, propertyString.Get(), true );
					szSourceString = outputScratchSpace.Get();
					return true;
				}
			}
			return false;
		};

	bool bReplacedAnything;
	do //loop to handle recursive indirection
	{
		bReplacedAnything = false;

		//MSVC
		{
			bReplacedAnything |= replaceWithString( "$(Configuration)", pRootConfig->m_Name );
			bReplacedAnything |= replaceWithString( "$(ConfigurationName)", pRootConfig->m_Name );
	
			bReplacedAnything |= replaceWithProperty( "$(IntDir)", KEYWORD_GENERAL, "$IntermediateDirectory" );
			bReplacedAnything |= replaceWithProperty( "$(IntermediateOutputPath)", KEYWORD_GENERAL, "$IntermediateDirectory" );
			

			bReplacedAnything |= replaceWithProperty( "$(OutDir)", KEYWORD_GENERAL, "$OutputDirectory" );
			bReplacedAnything |= replaceWithProperty( "$(OutputPath)", KEYWORD_GENERAL, "$OutputDirectory" );

			bReplacedAnything |= replaceWithProperty( "$(TargetName)", KEYWORD_LINKER, g_pOption_OutputFile );
			bReplacedAnything |= replaceWithProperty( "$(TargetName)", KEYWORD_LIBRARIAN, g_pOption_OutputFile );
		}
		
		//POSIX
		{
			bReplacedAnything |= replaceWithString( "${CONFIGURATION}", pRootConfig->m_Name );
			
			//replicated from src/devtools/makefile_base_posix.mak
			// At some point we should have VPC either read that from the file or drive the value directly from vpc scripts so they're guaranteed to be in sync
			if ( V_strstr( szSourceString, "$(OBJ_DIR)" ) != nullptr )
			{
				char szName[256];
				V_strncpy( szName, g_pVPC->GetProjectName(), sizeof(szName) );
				extern void MakeFriendlyProjectName( char *pchProject );
				MakeFriendlyProjectName( szName );

				//OBJ_DIR = ./obj_$(NAME)_$(TARGET_PLATFORM)$(TARGET_PLATFORM_EXT)/$(CFG)
				CUtlString objDirVal = "./obj_";
				objDirVal += szName;
				objDirVal += "_";
				objDirVal += g_pVPC->conditionals.GetTargetPlatformName();
				objDirVal += g_pVPC->IsDedicatedBuild() ? "" : "_client";
				objDirVal += "/";
				objDirVal += pRootConfig->m_LowerCaseName.Get();

				bReplacedAnything |= replaceWithString( "$(OBJ_DIR)", objDirVal );
			}
		}
	} while ( bReplacedAnything );

#if defined( DBGFLAG_ASSERT )
	{
		const char *pUnhandled = V_strstr( szSourceString, "$(" );
		if ( !pUnhandled )
		{
			pUnhandled = V_strstr( szSourceString, "%(" );
		}
		if ( !pUnhandled )
		{
			pUnhandled = V_strstr( szSourceString, "${" );
		}
		if ( pUnhandled )
		{
			CUtlString unhandled = pUnhandled;
			const char *pEndChar = ( pUnhandled[1] == '(' ) ? ")" : "{";
			const char *pUnhandledEnd = V_strstr( unhandled.Get(), pEndChar );
			if ( pUnhandledEnd )
			{
				++pUnhandledEnd;
				unhandled.SetLength( pUnhandledEnd - unhandled.Get() );
			}
			else
			{
				unhandled.Clear();
			}

			AssertMsg1( false, "Unhandled string replacement %s", unhandled.Get() );
		}
	}
#endif
	return szSourceString;
}


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void VPC_GetPreprocessorDefines( CProjectFile *pFile, CProjectConfiguration *pRootConfig, CUtlVector< CUtlString > &defines )
{
	defines.RemoveAll();

	CProjectConfiguration *pFileConfig = nullptr;
	if ( pFile )
	{
		pFile->GetConfiguration( pRootConfig->m_Name.Get(), &pFileConfig );
	}

	CUtlString cfgString;
	if ( !VPC_GetPropertyString( KEYWORD_COMPILER, pRootConfig, pFileConfig, g_pOption_PreprocessorDefinitions, &cfgString ) )
		return;

	g_pVPC->macros.GetPreprocessorDefines(cfgString.Get(), defines);

	// Remove surrounding whitespace & quotes (caller can add surrounding quotes if desired):
	CleanStrings(defines);
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void VPC_GetIncludeDirectories( CProjectFile *pFile, CProjectConfiguration *pRootConfig, CUtlVector< CUtlString > &includes )
{
	includes.RemoveAll();

	CProjectConfiguration *pFileConfig = nullptr;
	if ( pFile )
	{
		pFile->GetConfiguration( pRootConfig->m_Name.Get(), &pFileConfig );
	}

	CUtlString cfgString;
	if ( !VPC_GetPropertyString( KEYWORD_COMPILER, pRootConfig, pFileConfig, g_pOption_AdditionalIncludeDirectories, &cfgString ) )
		return;

    CUtlStringBuilder *pStrBuf = g_pVPC->GetTempStringBuffer1();
	CSplitString outStrings( cfgString.Get(), (const char**)g_IncludeSeparators, V_ARRAYSIZE(g_IncludeSeparators) );
	includes.EnsureCapacity( outStrings.Count() ); // Presize to avoid realloc'ing and copying strings
	for ( int i=0; i < outStrings.Count(); i++ )
	{
        pStrBuf->Set( outStrings[i] );
        pStrBuf->ReplaceFastCaseless( "$(IntDir)", "$(OBJ_DIR)" );
		V_FixSlashes( pStrBuf->Access(), '/' );
		includes.AddToTail( pStrBuf->Get() );
	}

	// Remove surrounding whitespace & quotes (caller can add surrounding quotes if desired):
	CleanStrings( includes );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void VPC_GetPCHInclude( CProjectFile *pFile, CProjectConfiguration *pRootConfig, CUtlString &pchFile, bool &bCreatesPCH, bool &bExcludesPCH )
{
	pchFile = "";
	bCreatesPCH = false;
	bExcludesPCH = false;

	CProjectConfiguration *pFileConfig = nullptr;
	if ( pFile )
	{
		pFile->GetConfiguration( pRootConfig->m_Name.Get(), &pFileConfig );
	}

	// Look for the $Create/UsePrecompiledHeader state, which should be set to either
	// "Create Precompiled Header (/Yc)" or "Use Precompiled Header (/Yu)":
	CUtlString cfgString;
	if ( !VPC_GetPropertyString( KEYWORD_COMPILER, pRootConfig, pFileConfig, g_pOption_PrecompiledHeader, &cfgString ) )
		return;
	if ( V_stristr( cfgString.Get(), "Create" ) )
		bCreatesPCH = true;
	else if ( V_stristr( cfgString.Get(), "Use" ) )
		bCreatesPCH = false;
	else
	{
		// Value is:  "Not Using Precompiled Headers"
		if ( pFileConfig )
		{
			// If the property came from pFileConfig, then this file is specifically configured to not use PCHs, so flag it as excluded:
			bExcludesPCH = !!VPC_GetToolProperty( KEYWORD_COMPILER, nullptr, pFileConfig, g_pOption_PrecompiledHeader );
		}
		return;
	}

	// Now look for the $Create/UsePCHThroughFile state, which specifies the name of the PCH file to include/create:
	if ( !VPC_GetPropertyString( KEYWORD_COMPILER, pRootConfig, pFileConfig, g_pOption_UsePCHThroughFile, &pchFile ) )
	{
		const char *pFilename = pFile ? pFile->m_Name.Get() : "unknown";
		if ( bCreatesPCH )
		{
			// Emit a warning so we can ensure VPCs provide explicit information about how each PCH file is built:
			logging::Warning( "File (%s) creates a PCH (/Yc), but has no 'UsePCHThroughFile' command, so there is no way to tell *which* PCH it creates!\n", pFilename );
			bCreatesPCH = false;
		}
		else
		{
			// This is also an error case: it says 'use PCH' but doesn't specify *which* PCH to use!:
			// TODO: Work out if this warning is needed.
			//logging::Warning( "File (%s) uses a PCH (/Yu), but has no 'UsePCHThroughFile' command, so there is no way to tell *which* PCH it uses!\n", pFilename );
		}
		return;
	}

	pchFile.FixSlashes( '/' );
}


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void VPC_GeneratePCHInfoList_ProcessFile(	CProjectFile *pFile, CProjectConfiguration *pRootConfig,
											CUtlVector< CUtlString > &pchIncludeNames, CUtlVector< CUtlString > &pchCreatorNames, CUtlVector< CUtlString > &usedPCHs,
											CUtlVector< CProjectFile * > *pFilesExcludingPCH )
{
	bool bFileCreatesPCH, bFileExcludesPCH;
	CUtlString pchName;
	VPC_GetPCHInclude( pFile, pRootConfig, pchName, bFileCreatesPCH, bFileExcludesPCH );
	if ( pFilesExcludingPCH && bFileExcludesPCH )
	{
		Assert( !pFilesExcludingPCH->HasElement( pFile ) );
		pFilesExcludingPCH->AddToTail( pFile );
	}
	if ( bFileCreatesPCH )
	{
		if ( pchIncludeNames.HasElement( pchName ) )
		{
			// Each PCH should only be created by one source file!
			int index = pchIncludeNames.Find( pchName );
			NOTE_UNUSED( index );
			AssertMsg4( !pchIncludeNames.HasElement( pchName ), "ERROR (%s): PCH file '%s' created by two separate files in the same VPC! ('%s' and '%s')\n",
						__FUNCTION__, pchName.Get(), pFile->m_Name.Get(), pchCreatorNames[ index ].Get() );
			return;
		}
		pchIncludeNames.AddToTail( pchName );
		pchCreatorNames.AddToTail( pFile->m_Name );
		pchIncludeNames.Tail().FixSlashes( '/' );
		pchCreatorNames.Tail().FixSlashes( '/' );
	}
	else if ( !pchName.IsEmpty() )
	{
		if ( !usedPCHs.HasElement( pchName ) )
			usedPCHs.AddToTail( pchName );
	}
}


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void VPC_GeneratePCHInfoList_ProcessFolder(	CProjectFolder *pFolder, CProjectConfiguration *pRootConfig,
											CUtlVector< CUtlString > &pchIncludeNames, CUtlVector< CUtlString > &pchCreatorNames, CUtlVector< CUtlString > &usedPCHs,
											CUtlVector< CProjectFile * > *pFilesExcludingPCH )
{
	for ( int iIndex = pFolder->m_Files.Head(); iIndex != pFolder->m_Files.InvalidIndex(); iIndex = pFolder->m_Files.Next( iIndex ) )
	{
		VPC_GeneratePCHInfoList_ProcessFile( pFolder->m_Files[iIndex], pRootConfig, pchIncludeNames, pchCreatorNames, usedPCHs, pFilesExcludingPCH );
	}
	for ( int iIndex = pFolder->m_Folders.Head(); iIndex != pFolder->m_Folders.InvalidIndex(); iIndex = pFolder->m_Folders.Next( iIndex ) )
	{
		VPC_GeneratePCHInfoList_ProcessFolder( pFolder->m_Folders[iIndex], pRootConfig, pchIncludeNames, pchCreatorNames, usedPCHs, pFilesExcludingPCH );
	}
}


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void VPC_GeneratePCHInfo(	CVCProjGenerator *pDataCollector, CProjectConfiguration *pRootConfig,
							CUtlVector< CUtlString > &pchIncludeNames, CUtlVector< CUtlString > &pchCreatorNames,
							CUtlVector< CUtlString > const *pRequiredPCHs, CUtlVector< CProjectFile * > *pFilesExcludingPCH )
{
	// Generate a list of all PCH headers used by this project, with the corresponding source files used to build the PCHs
	Assert( pRootConfig );

	CUtlVector< CUtlString > usedPCHs; // Which PCH files are included by source files in the project?
	pchIncludeNames.RemoveAll();
	pchCreatorNames.RemoveAll();
	VPC_GeneratePCHInfoList_ProcessFolder( pDataCollector->GetRootFolder(), pRootConfig, pchIncludeNames, pchCreatorNames, usedPCHs, pFilesExcludingPCH );

	// Error-check: all used PCHs must actually be created!
	for ( int i = 0; i < pchIncludeNames.Count(); i++ )
	{
		usedPCHs.FindAndFastRemove( pchIncludeNames[i] );
	}
	for ( int i = 0; i < usedPCHs.Count(); i++ )
	{
		logging::Warning( "[VPC_GeneratePCHInfo] Could not find VPC entry which generates PCH file for PCH header %s, in project %s!\n", usedPCHs[i].Get(), pDataCollector->GetProjectName() );
		DebuggerBreakIfDebugging();
	}

	if ( pRequiredPCHs )
	{
		// Filter out superfluous infos, based on the given list of required PCHs:
		for ( int i = 0; i < pchIncludeNames.Count(); i++ )
		{
			if ( !pRequiredPCHs->HasElement( pchIncludeNames[i] ) )
			{
				pchIncludeNames.FastRemove( i );
				pchCreatorNames.FastRemove( i );
				i--;
			}
		}
	}
}
