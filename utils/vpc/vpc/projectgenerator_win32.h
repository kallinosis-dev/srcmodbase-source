//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef PROJECTGENERATOR_WIN32_H
#define PROJECTGENERATOR_WIN32_H
#ifdef _WIN32
#pragma once
#endif

#define PROPERTYNAME( X, Y ) X##_##Y,
enum Win32Properties_e
{
	#include "projectgenerator_win32.inc"
};

class CProjectGenerator_Win32 : public IVCProjWriter
{
public:
	CProjectGenerator_Win32();
	CVCProjGenerator *GetProjectGenerator() override { return m_pVCProjGenerator; }

	bool Save( const char *pOutputFilename ) override;

private:
	bool		WriteToXML();

	bool		WriteFolder( CProjectFolder *pFolder );
	bool		WriteFile( CProjectFile *pFile );
	bool		WriteConfiguration( CProjectConfiguration *pConfig );
	bool		WriteProperty( const PropertyState_t *pPropertyState, const char *pOutputName = nullptr, const char *pValue = nullptr);
	bool		WriteTool( const char *pToolName, const CProjectTool *pProjectTool );
	bool		WriteNULLTool( const char *pToolName, const CProjectConfiguration *pConfig );

	CXMLWriter			m_XMLWriter;
	CVCProjGenerator	*m_pVCProjGenerator;
};

#endif // PROJECTGENERATOR_WIN32_H
