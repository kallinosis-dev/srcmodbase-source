#pragma once
#include "scriptsource.h"
#include "tier0/logging.h"

class IScope;

DECLARE_LOGGING_CHANNEL(LOG_VPC);


namespace logging
{
	void		Init();
	void		Shutdown();

	void		SetVerbose(bool verbose);
	bool		IsVerbose();

	void		SetQuiet(bool quiet);
	bool		IsQuiet();

	// FIXME: hack, should not filter out log messages
	void		SetIgnoreRedundancyWarning(bool ignore);
	bool		IsIgnoreRedundancyWarning();

	namespace pacifier
	{
		void		Clear();
		void		Output();
		void		Break();
	}
}

struct CDebugContext {
	char /*nullable*/ const* name;
	CScript /*nullable*/ const* script;
	IScope /*nullable*/ const* scope;
};

// Use a-la RAII wrapper, there isn't much reason to save it into fields.
//
// void SomethingWithACtx::DoStuff() {
//      CContextualLogger log { this->GetDebugCtx() };
//
// 
//      log.Status("Something happened");
//      log.Warning("Something bad %s", "happened");
//      log.Error("Some bullshit(tm) happened. We're done.");
// }
class CContextualLogger {
public:
	CContextualLogger(CDebugContext const* ctx);
	
	void SetWriteScriptPosition(bool value);
	bool GetWriteScriptPosition() const;

	void SetWriteName(bool value);
	bool GetWriteName() const;
	
	void VerboseStatus(PRINTF_FORMAT_STRING char const* fmt, ...) const FMTFUNCTION(2, 3);
	void VerboseStatusColored(Color color, PRINTF_FORMAT_STRING char const* fmt, ...) const FMTFUNCTION(3, 4);
	
	void Status(PRINTF_FORMAT_STRING char const* fmt, ...) const FMTFUNCTION(2, 3);
	void StatusColored(Color color, PRINTF_FORMAT_STRING char const* fmt, ...) const FMTFUNCTION(3, 4);
	
	void Warning(PRINTF_FORMAT_STRING char const* fmt, ...) const FMTFUNCTION(2, 3);

	[[noreturn]] void Error(PRINTF_FORMAT_STRING char const* fmt, ...) const FMTFUNCTION(2, 3);

	
	void DumpContext() const;

private:
	static char const* HandlePrefixNewlines(char const* str);
	void PrintPrefix(LoggingSeverity_t level, Color color) const;
	

private:
	CDebugContext const* _ctx;
	bool _writeScriptPosition = false;
	bool _writeName = true;
};