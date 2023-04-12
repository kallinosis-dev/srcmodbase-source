
#include "base.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#include "tier0/logging.h"
#include "tier0/dbg.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS(LOG_HAVOK, "Havok");

hk_Console *hk_Console::m_console = nullptr;
hk_Console hk_Console::m_default_console_buffer;

hk_Console *hk_Console::get_instance()
{
	if ( !m_console )
	{
		m_console = &m_default_console_buffer;
	}
	return m_console;
}
#define MAX_ERROR_BUFFER_LEN 2048

void hk_Console::printf( const char *fmt, ...)
{
	char buffer[MAX_ERROR_BUFFER_LEN];

	va_list vlist;
	va_start(vlist, fmt);
	//XXX fixme
	// scan format list for havok vector matrix tokens %V %M
	vsprintf(buffer, fmt, vlist);
	va_end(vlist);

	Log_Warning(LOG_HAVOK, "%s", buffer);
}

void hk_Console::flush()
{

}


void hk_Console::exit( int code )
{
	AssertMsg1(false, "Havok would terminate program with code %i", code);
}
