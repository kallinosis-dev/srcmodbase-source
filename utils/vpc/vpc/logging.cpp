#include "logging.h"

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


// --------------------------------------------

[[noreturn]] void logging::Error(const char* pFormat, ...)
{
	va_list argptr;
	char msg[MAX_SYSPRINTMSG];

	va_start(argptr, pFormat);
	vsprintf(msg, pFormat, argptr);
	va_end(argptr);

	pacifier::Break();

	// since we are going to prefix want caller provided prefixed CR to be handled first to keep message intact
	const char* pMsg = msg;
	while (*pMsg == '\n')
	{
		Log_Warning(LOG_VPC, Color(255, 0, 0, 255), "\n");
		pMsg++;
	}

	// spew in red
	Log_Warning(LOG_VPC, Color(255, 0, 0, 255), "ERROR: %s\n", msg);

	// dump the script stack to assist in user understanding of the include chain
	g_pVPC->GetScript().SpewScriptStack(true);

	// stop here if debugging
	DebuggerBreakIfDebugging();

	// do proper shutdown in an error context
	// errors are expected to be fatal by all calling code
	// otherwise it would have been a warning
	g_pVPC->Shutdown(true);

	UNREACHABLE();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void logging::SyntaxError(const char* pFormat, ...)
{
	va_list argptr;
	char msg[MAX_SYSPRINTMSG];

	va_start(argptr, pFormat);
	if (pFormat)
	{
		vsprintf(msg, pFormat, argptr);
	}
	va_end(argptr);

	pacifier::Break();

	if (pFormat)
	{
		// since we are going to prefix want caller provided prefixed CR to be handled first to keep message intact
		const char* pMsg = msg;
		while (*pMsg == '\n')
		{
			Log_Warning(LOG_VPC, Color(255, 0, 0, 255), "\n");
			pMsg++;
		}

		Log_Warning(LOG_VPC, Color(255, 0, 0, 255), "Bad Syntax: %s\n", pMsg);
	}

	CScript const& script = g_pVPC->GetScript();

	// syntax errors are fatal
	Error("Bad Syntax in '%s' line:%d\n", script.GetName(), script.GetLine());
}

void logging::Warning(const char* pFormat, ...)
{
	va_list argptr;
	char msg[MAX_SYSPRINTMSG];

	va_start(argptr, pFormat);
	vsprintf(msg, pFormat, argptr);
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

	pacifier::Break();

	// since we are going to prefix want caller provided prefixed CR to be handled first to keep message intact
	const char* pMsg = msg;
	while (*pMsg == '\n')
	{
		Log_Warning(LOG_VPC, Color(255, 255, 0, 255), "\n");
		pMsg++;
	}

	Log_Warning(LOG_VPC, Color(255, 255, 0, 255), "WARNING: %s\n", pMsg);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void logging::Status(bool bAlwaysSpew, const char* pFormat, ...)
{
	if (g_quiet || (!bAlwaysSpew && !g_verbose))
		return;

	va_list argptr;
	char msg[MAX_SYSPRINTMSG];

	va_start(argptr, pFormat);
	vsprintf(msg, pFormat, argptr);
	va_end(argptr);

	pacifier::Break();

	// since we auto suffix CR, prevent dual CR when caller just wants a single CR
	const char* pMsg = msg;
	while (*pMsg == '\n')
	{
		Log_Msg(LOG_VPC, "\n");
		pMsg++;
	}

	if (pMsg[0])
	{
		Log_Msg(LOG_VPC, "%s\n", pMsg);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void logging::StatusWithColor(bool bAlwaysSpew, Color messageColor, const char* pFormat, ...)
{
	if (g_quiet || (!bAlwaysSpew && !g_verbose))
		return;

	va_list argptr;
	char msg[MAX_SYSPRINTMSG];

	va_start(argptr, pFormat);
	vsprintf(msg, pFormat, argptr);
	va_end(argptr);

	pacifier::Break();

	// since we auto suffix CR, prevent dual CR when caller just wants a single CR
	const char* pMsg = msg;
	while (*pMsg == '\n')
	{
		Log_Msg(LOG_VPC, messageColor, "\n");
		pMsg++;
	}

	if (pMsg[0])
	{
		Log_Msg(LOG_VPC, messageColor, "%s\n", pMsg);
	}
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