#include "ansi_3_64.h"


namespace lyramilk{namespace ansi_3_64{
	std::ostream& reset(std::ostream& os)
	{
		os << "\x1b[0m";
		return os;
	}

	std::ostream& blod(std::ostream& os)
	{
		os << "\x1b[1m";
		return os;
	}

	std::ostream& black(std::ostream& os)
	{
		os << "\x1b[30m";
		return os;
	}

	std::ostream& red(std::ostream& os)
	{
		os << "\x1b[31m";
		return os;
	}

	std::ostream& green(std::ostream& os)
	{
		os << "\x1b[32m";
		return os;
	}

	std::ostream& yellow(std::ostream& os)
	{
		os << "\x1b[33m";
		return os;
	}

	std::ostream& blue(std::ostream& os)
	{
		os << "\x1b[34m";
		return os;
	}

	std::ostream& magenta(std::ostream& os)
	{
		os << "\x1b[35m";
		return os;
	}

	std::ostream& cyan(std::ostream& os)
	{
		os << "\x1b[36m";
		return os;
	}

	std::ostream& white(std::ostream& os)
	{
		os << "\x1b[37m";
		return os;
	}

	std::ostream& underline(std::ostream& os)
	{
		os << "\x1b[4m";
		return os;
	}

	std::ostream& underline_endl(std::ostream& os)
	{
		os << "\x1b[24m";
		return os;
	}

	std::ostream& blink_slow(std::ostream& os)
	{
		os << "\x1b[5m";
		return os;
	}

	std::ostream& blink_rapid(std::ostream& os)
	{
		os << "\x1b[6m";
		return os;
	}

	std::ostream& blink_endl(std::ostream& os)
	{
		os << "\x1b[25m";
		return os;
	}


}}