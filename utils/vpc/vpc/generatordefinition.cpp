//========= Copyright � 1996-2016, Valve Corporation, All rights reserved. ============//
//
//
//=====================================================================================//

#include "generatordefinition.h"

#include "tier1/fmtstr.h"
#include "tier1/keyvalues.h"
#include "vpc.h"

void CGeneratorDefinition::IterateAttributesKey( ToolProperty_t *pProperty, KeyValues *pAttributesKV )
{
	const char *pAttributeName = pAttributesKV->GetName();
	const char *pValue = pAttributesKV->GetString( "" );

	//Msg( "Attribute name: %s\n", pAttributeName );

	if ( !V_stricmp_fast( pAttributeName, "type" ) )
	{
		if ( !V_stricmp_fast( pValue, "bool" ) || !V_stricmp_fast( pValue, "boolean" ) )
		{
			pProperty->m_nType = PT_BOOLEAN;
		}
		else if ( !V_stricmp_fast( pValue, "string" ) )
		{
			pProperty->m_nType = PT_STRING;
		}
		else if ( !V_stricmp_fast( pValue, "list" ) )
		{
			pProperty->m_nType = PT_LIST;
		}
		else if (  !V_stricmp_fast( pValue, "int" ) || !V_stricmp_fast( pValue, "integer" ) )
		{
			pProperty->m_nType = PT_INTEGER;
		}
		else if ( !V_stricmp_fast( pValue, "ignore" ) || !V_stricmp_fast( pValue, "none" ) )
		{
			pProperty->m_nType = PT_IGNORE;
		}
		else if ( !V_stricmp_fast( pValue, "deprecated" ) || !V_stricmp_fast( pValue, "donotuse" ) )
		{
			pProperty->m_nType = PT_DEPRECATED;
		}
		else 
		{
			// unknown
			logging::Error( "Unknown type '%s' in '%s'", pValue, pProperty->m_ParseString.Get() );
		}
	}
	else if ( !V_stricmp_fast( pAttributeName, "alias" ) )
	{
		pProperty->m_AliasString = pValue;
	}
	else if ( !V_stricmp_fast( pAttributeName, "legacy" ) )
	{
		pProperty->m_LegacyString = pValue;
	}
	else if ( !V_stricmp_fast( pAttributeName, "InvertOutput" ) )
	{
		pProperty->m_bInvertOutput = pAttributesKV->GetBool();
	}
	else if ( !V_stricmp_fast( pAttributeName, "output" ) )
	{
		pProperty->m_OutputString = pValue;
	}
	else if ( !V_stricmp_fast( pAttributeName, "fixslashes" ) )
	{
		pProperty->m_bFixSlashes = pAttributesKV->GetBool();
	}
	else if ( !V_stricmp_fast( pAttributeName, "PreferSemicolonNoComma" ) )
	{
		pProperty->m_bPreferSemicolonNoComma = pAttributesKV->GetBool();
	}
	else if ( !V_stricmp_fast( pAttributeName, "PreferSemicolonNoSpace" ) )
	{
		pProperty->m_bPreferSemicolonNoSpace = pAttributesKV->GetBool();
	}
	else if ( !V_stricmp_fast( pAttributeName, "AppendSlash" ) )
	{
		pProperty->m_bAppendSlash = pAttributesKV->GetBool();
	}
	else if ( !V_stricmp_fast( pAttributeName, "GlobalProperty" ) )
	{
		pProperty->m_bEmitAsGlobalProperty = pAttributesKV->GetBool();
	}
	else if ( !V_stricmp_fast( pAttributeName, "BreakOnRead" ) )
	{
		pProperty->m_bBreakOnRead = pAttributesKV->GetBool();
	}
	else if ( !V_stricmp_fast( pAttributeName, "BreakOnWrite" ) )
	{
		pProperty->m_bBreakOnWrite = pAttributesKV->GetBool();
	}
	else if ( !V_stricmp_fast( pAttributeName, "ordinals" ) )
	{
		if ( pProperty->m_nType == PT_UNKNOWN )
		{
			pProperty->m_nType = PT_LIST;
		}

		for ( KeyValues *pKV = pAttributesKV->GetFirstSubKey(); pKV; pKV = pKV->GetNextKey() )
		{
			const char *pOrdinalName = pKV->GetName();
			const char *pOrdinalValue = pKV->GetString();
			if ( !pOrdinalValue[0] )
			{
				logging::Error( "Unknown ordinal value for name '%s' in '%s'", pOrdinalName, pProperty->m_ParseString.Get() );
			}

			int iIndex = pProperty->m_Ordinals.AddToTail();
			pProperty->m_Ordinals[iIndex].m_ParseString = pOrdinalName;
			pProperty->m_Ordinals[iIndex].m_ValueString = pOrdinalValue;
		}
	}
	else if ( !V_stricmp_fast( pAttributeName, "IgnoreForOutput" ) )
	{
		pProperty->m_bIgnoreForOutput = pAttributesKV->GetBool();
	}
	else if ( !V_stricmp_fast( pAttributeName, "GeneratedOnOutput" ) )
	{
		pProperty->m_bGeneratedOnOutput = pAttributesKV->GetBool();
	}
	else
	{
		logging::Error( "Unknown attribute '%s' in '%s'", pAttributeName, pProperty->m_ParseString.Get() );
	}
}

void CGeneratorDefinition::IteratePropertyKey( GeneratorTool_t *pTool, KeyValues *pPropertyKV )
{
	//Msg( "Property Key name: %s\n", pPropertyKV->GetName() );

	int iIndex = pTool->m_Properties.AddToTail();
	ToolProperty_t *pProperty = &pTool->m_Properties[iIndex];

	pProperty->m_ParseString = pPropertyKV->GetName();

	KeyValues *pKV = pPropertyKV->GetFirstSubKey();
	if ( !pKV )
		return;

	for ( ;pKV; pKV = pKV->GetNextKey() )
	{		
		IterateAttributesKey( pProperty, pKV );
	}
}

void CGeneratorDefinition::IterateToolKey( KeyValues *pToolKV )
{
	//Msg( "Tool Key name: %s\n", pToolKV->GetName() );

	// find or create
	GeneratorTool_t *pTool = nullptr;
	for ( int i = 0; i < m_Tools.Count(); i++ )
	{
		if ( !V_stricmp_fast( pToolKV->GetName(), m_Tools[i].m_ParseString.Get() ) )
		{
			pTool = &m_Tools[i];
			break;
		}
	}
	if ( !pTool )
	{
		int iIndex = m_Tools.AddToTail();
		pTool = &m_Tools[iIndex];
	}

	pTool->m_ParseString = pToolKV->GetName();

	KeyValues *pKV = pToolKV->GetFirstSubKey();
	if ( !pKV )
		return;

	for ( ;pKV; pKV = pKV->GetNextKey() )
	{		
		IteratePropertyKey( pTool, pKV );
	}
}

void CGeneratorDefinition::AssignIdentifiers()
{
	CUtlVector< bool > usedPropertyNames;
	int nTotalPropertyNames = 0;
	while ( m_pPropertyNames[nTotalPropertyNames].m_nPropertyId >= 0 )
	{
		nTotalPropertyNames++;
	}
	usedPropertyNames.SetCount( nTotalPropertyNames );

	// assign property identifiers
	for ( int i = 0; i < m_Tools.Count(); i++ )
	{
		GeneratorTool_t *pTool = &m_Tools[i];

		// assign the tool keyword
		configKeyword_e keyword = g_pVPC->NameToKeyword( pTool->m_ParseString.Get() );
		if ( keyword == KEYWORD_UNKNOWN )
		{
			logging::Error( "Unknown Tool Keyword '%s' in '%s'", pTool->m_ParseString.Get(), m_ScriptName.Get() );
		}
		pTool->m_nKeyword = keyword;

		const char *pPrefix = m_NameString.Get();
		const char *pToolName = pTool->m_ParseString.Get();
		if ( pToolName[0] == '$' )
		{
			pToolName++;
		}
		
		CUtlString prefixString = CFmtStr( "%s_%s", pPrefix, pToolName ).Get();

		for ( int j = 0; j < pTool->m_Properties.Count(); j++ )
		{
			ToolProperty_t *pProperty = &pTool->m_Properties[j];

			if ( pProperty->m_nType == PT_IGNORE || pProperty->m_nType == PT_DEPRECATED )
			{
				continue;
			}

			const char *pPropertyName = pProperty->m_AliasString.Get();
			if ( !pPropertyName[0] )
			{
				pPropertyName = pProperty->m_ParseString.Get();
			}
			if ( pPropertyName[0] == '$' )
			{
				pPropertyName++;
			}

			bool bFound = false;
			for ( int k = 0; k < nTotalPropertyNames && !bFound; k++ )
			{
				if ( !V_stricmp_fast( prefixString.Get(), m_pPropertyNames[k].m_pPrefixName ) )
				{
					if ( !V_stricmp_fast( pPropertyName, m_pPropertyNames[k].m_pPropertyName ) )
					{
						pProperty->m_nPropertyId = m_pPropertyNames[k].m_nPropertyId;
						bFound = true;
						usedPropertyNames[k] = true;
					}
				}
			}
			if ( !bFound )
			{
				logging::Error( "Could not find PROPERTYNAME( %s, %s ) for %s", prefixString.Get(), pPropertyName, m_ScriptName.Get() );
			}
		}
	}

	if ( logging::IsVerbose() )
	{
		for ( int i = 0; i < usedPropertyNames.Count(); i++ )
		{
			if ( !usedPropertyNames[i] )
			{
				logging::Warning( "Unused PROPERTYNAME( %s, %s ) in %s", m_pPropertyNames[i].m_pPrefixName, m_pPropertyNames[i].m_pPropertyName, m_ScriptName.Get() );
			}
		}
	}
}

CGeneratorDefinition::CGeneratorDefinition( const char *pDefinitionName, PropertyName_t *pPropertyNames )
{
	m_VersionString.Clear();
	m_Tools.Purge();

	CUtlPathStringHolder scriptFilename( g_pVPC->GetSourcePath(), "\\vpc_scripts\\definitions\\", pDefinitionName );
	scriptFilename.FixSlashes();

	m_pPropertyNames = pPropertyNames;

	CScript script;
	script.PushScript( scriptFilename.Get() );
	
	// project definitions are KV format
	KeyValues *pScriptKV = new KeyValues( script.GetName() );

	pScriptKV->LoadFromBuffer( script.GetName(), script.GetData() );

	m_ScriptName = script.GetName();
	m_ScriptCRC = CRC32_ProcessSingleBuffer( script.GetData(), V_strlen( script.GetData() ) );

	m_NameString = pScriptKV->GetName();

	KeyValues *pKV = pScriptKV->GetFirstSubKey();
	for ( ;pKV; pKV = pKV->GetNextKey() )
	{
		const char *pKeyName = pKV->GetName();
		if ( !V_stricmp_fast( pKeyName, "version" ) )
		{
			m_VersionString = pKV->GetString();
		}
		else
		{
			IterateToolKey( pKV );
		}
	}

	script.PopScript();
	pScriptKV->deleteThis();

	logging::Status( false, "Definition: '%s' Version: %s", m_NameString.Get(), m_VersionString.Get() );

	AssignIdentifiers();
}

const char *CGeneratorDefinition::GetScriptName( CRC32_t *pCRC )
{
	if ( pCRC )
	{
		*pCRC = m_ScriptCRC;
	}

	return m_ScriptName.Get();
}

ToolProperty_t *CGeneratorDefinition::GetProperty( configKeyword_e keyword, const char *pPropertyName )
{
	for ( int i = 0; i < m_Tools.Count(); i++ )
	{
		GeneratorTool_t *pTool = &m_Tools[i];
		if ( pTool->m_nKeyword != keyword )
			continue;

		for ( int j = 0; j < pTool->m_Properties.Count(); j++ )
		{
			// TODO: use a symbol table instead of stricmp (in source2 'VPC +everything' does 16.3 million stricmps here, likely wasting several seconds)
			ToolProperty_t *pToolProperty = &pTool->m_Properties[j];
			if ( !V_stricmp_fast( pToolProperty->m_ParseString.Get(), pPropertyName ) )
			{
				// found
				return pToolProperty;
			}
			if ( !pToolProperty->m_LegacyString.IsEmpty() && !V_stricmp_fast( pToolProperty->m_LegacyString.Get(), pPropertyName ) )
			{
				// found
				return pToolProperty;
			}
		}
	}

	// not found
	return nullptr;
}


















	
