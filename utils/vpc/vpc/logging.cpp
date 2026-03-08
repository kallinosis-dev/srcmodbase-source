#include "logging.h"

#include "scope.h"
#include "vpc.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS(LOG_VPC, "VPC");



static bool g_verbose = false;
static bool g_quiet = false;
static bool g_ignoreRedundancyWarning = false;

void logging::SetVerbose(bool verbose)
{
	g_verbose = verbose;
}

void logging::SetQuiet(bool quiet)
{
	g_quiet = quiet;
}

bool logging::IsVerbose()
{
	return g_verbose;
}

bool logging::IsQuiet()
{
	return g_quiet;
}

void logging::SetIgnoreRedundancyWarning(bool ignore)
{
	g_ignoreRedundancyWarning = ignore;
}

bool logging::IsIgnoreRedundancyWarning()
{
	return g_ignoreRedundancyWarning;
}

static CColorizedLoggingListener	g_loggingListener;

void logging::Init()
{
	// We don't really need to pop the logging state since the process will terminate when we're done.
	LoggingSystem_PushLoggingState();

	g_loggingListener.m_bQuietPrintf = g_quiet;
	LoggingSystem_RegisterLoggingListener(&g_loggingListener);
}

void logging::Shutdown()
{
	LoggingSystem_UnregisterLoggingListener(&g_loggingListener);
}

//--------------------------------------------------------------------------------
static int g_nPacifier = 0;


void logging::pacifier::Clear()
{
	g_nPacifier = 0;
}

void logging::pacifier::Output()
{
	if (!(g_nPacifier++ % 40) && (g_nPacifier > 1))
	{
		// break rows of pacifiers
		Log_Msg(LOG_VPC, "\n");
	}

	// Add another dot for the pacifier.
	Log_Msg(LOG_VPC, ".");
}

void logging::pacifier::Break()
{
	if (g_nPacifier)
	{
		Log_Msg(LOG_VPC, "\n");
		g_nPacifier = 0;
	}
}

//--------------------------------------------------------------------------------

CDebugContext g_DefaultDebugContext { .name = nullptr };

static Color clr_status = Color(255, 255, 255, 255);
static Color clr_warn = Color(255, 255, 0, 255);
static Color clr_err = Color(255, 0, 0, 255);


void CContextualLogger::VerboseStatus(char const* fmt, ...) const
{
	if(g_quiet || !g_verbose) return;

	va_list argptr;
	char msg[MAX_SYSPRINTMSG];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	logging::pacifier::Break();

	char const* finalMsg = HandlePrefixNewlines(msg);

	PrintPrefix(LS_MESSAGE, clr_status);
	Log_Msg(LOG_VPC, clr_status, "%s\n", finalMsg);
}

void CContextualLogger::VerboseStatus(Color color, char const* fmt, ...) const
{
	if(g_quiet || !g_verbose) return;

	va_list argptr;
	char msg[MAX_SYSPRINTMSG];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	logging::pacifier::Break();

	char const* finalMsg = HandlePrefixNewlines(msg);

	PrintPrefix(LS_MESSAGE, color);
	Log_Msg(LOG_VPC, color, "%s\n", finalMsg);
}

void CContextualLogger::Status(char const* fmt, ...) const
{
	if(g_quiet) return;

	va_list argptr;
	char msg[MAX_SYSPRINTMSG];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	logging::pacifier::Break();

	char const* finalMsg = HandlePrefixNewlines(msg);

	PrintPrefix(LS_MESSAGE, clr_status);
	Log_Msg(LOG_VPC, clr_status, "%s\n", finalMsg);
}

void CContextualLogger::Status(Color color, char const* fmt, ...) const
{
	if(g_quiet) return;

	va_list argptr;
	char msg[MAX_SYSPRINTMSG];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	logging::pacifier::Break();

	char const* finalMsg = HandlePrefixNewlines(msg);

	PrintPrefix(LS_MESSAGE, color);
	Log_Msg(LOG_VPC, color, "%s\n", finalMsg);
}

void CContextualLogger::Warning(char const* fmt, ...) const
{
	va_list argptr;
	char msg[MAX_SYSPRINTMSG];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	if (g_ignoreRedundancyWarning)
	{
		if (V_stristr(msg, "matches default setting"))
			return;
		if (V_stristr(msg, "already exists in project"))
			return;
		if (V_stristr(msg, "specified multiple times"))
			return;
	}

	logging::pacifier::Break();

	char const* finalMsg = HandlePrefixNewlines(msg);

	PrintPrefix(LS_WARNING, clr_warn);
	Log_Warning(LOG_VPC, clr_warn, "%s\n", finalMsg);
}

void CContextualLogger::Error(char const* fmt, ...) const
{
	va_list argptr;
	char msg[MAX_SYSPRINTMSG];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	logging::pacifier::Break();

	char const* finalMsg = HandlePrefixNewlines(msg);

	PrintPrefix(LS_ERROR, clr_err);
	Log_Error(LOG_VPC, clr_err, "%s\n\n", finalMsg);

	DumpContext();

	DebuggerBreakIfDebugging();

	// do proper shutdown in an error context
	// errors are expected to be fatal by all calling code
	// otherwise it would have been a warning
	g_pVPC->Shutdown(true);
	UNREACHABLE();
}

//--------------------------------------------------------------------------------

CContextualLogger::CContextualLogger(CDebugContext const* ctx): _ctx(ctx ? ctx : &g_DefaultDebugContext)
{}

void CContextualLogger::SetWriteScriptPosition(bool value)
{
	_writeScriptPosition = value;
}

bool CContextualLogger::GetWriteScriptPosition() const
{
	return _writeScriptPosition;
}

void CContextualLogger::SetWriteName(bool value)
{
	_writeName = value;
}

bool CContextualLogger::GetWriteName() const
{
	return _writeName;
}


char const* CContextualLogger::HandlePrefixNewlines(char const* str)
{
	while(*str == '\n')
	{
		Log_Msg(VPC, "\n");
		++str;
	}

	return str;
}

void CContextualLogger::PrintPrefix(LoggingSeverity_t level, Color color) const
{
	if(_writeName)
	{
		char const* name1 = _ctx->scope ? _ctx->scope->GetName() : nullptr;
		char const* name2 = _ctx->name;

		if(name1 == nullptr && name2 != nullptr)
		{
			name1 = name2;
			name2 = nullptr;
		}

		if(name1 && name2)
			InternalMsg(LOG_VPC, level, color, "[%s - %s] ", name1, name2);
		else if(name1)
			InternalMsg(LOG_VPC, level, color, "[%s] ", name1);	
	}

	if(_writeScriptPosition && _ctx->script)
	{
		InternalMsg(LOG_VPC, level, color, "[%s:%i] ", _ctx->script->GetName(), _ctx->script->GetLine());
	}
}

void CContextualLogger::DumpContext() const
{
	IScope const* scope = _ctx->scope;
	CScript const* script = _ctx->script;

	if(scope == nullptr && script == nullptr)
	{
		Log_Msg(LOG_VPC, "<No context available>\n");
		return;
	}

	if(scope != nullptr)
	{
		scope->DumpState();
		Log_Msg(LOG_VPC, "\n");
	}

	if(script != nullptr)
		script->SpewScriptStack(false);
}
