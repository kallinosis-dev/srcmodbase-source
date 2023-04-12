#ifndef HK_BASE_CONSOLE_H
#define HK_BASE_CONSOLE_H

class hk_Console 
{
	public:

		void Log_Warning(LOG_HAVOK,  const char *format_string, ...);
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

