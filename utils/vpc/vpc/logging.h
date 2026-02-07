#pragma once
#include "scriptsource.h"
#include "tier0/logging.h"


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


		[[noreturn]]
	void		Error(PRINTF_FORMAT_STRING const char* pFormat, ...) FMTFUNCTION(2, 3);
	void		SyntaxError(CScript const* script,PRINTF_FORMAT_STRING const char* pFormat = nullptr, ...) FMTFUNCTION(2, 3);

	void		Warning(PRINTF_FORMAT_STRING const char* pFormat, ...) FMTFUNCTION(2, 3);

	void		Status(bool bAlwaysSpew, PRINTF_FORMAT_STRING const char* pFormat, ...) FMTFUNCTION(3, 4);
	void		StatusWithColor(bool bAlwaysSpew, Color messageColor, PRINTF_FORMAT_STRING const char* pFormat, ...) FMTFUNCTION(4, 5);

	namespace pacifier
	{
		void		Clear();
		void		Output();
		void		Break();
	}
}

