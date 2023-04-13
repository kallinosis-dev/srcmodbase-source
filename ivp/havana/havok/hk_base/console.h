#ifndef HK_BASE_CONSOLE_H
#define HK_BASE_CONSOLE_H
#include "tier0/logging.h"

DECLARE_LOGGING_CHANNEL(LOG_HAVOK);

class hk_Console 
{
	public:

		void printf(const char* fmt, ...);
		void exit(int );
		void flush();

		static hk_Console* get_instance();

	protected:

		static hk_Console* m_console;
		static hk_Console m_default_console_buffer;
			//: just to avoid a call to malloc
};

#define hkprintf hk_Console::get_instance()->printf
#endif /* HK_BASE_CONSOLE_H */

