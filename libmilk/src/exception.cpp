#include "exception.h"

namespace lyramilk{
	exception::exception()throw()
	{
	}

	exception::exception(lyramilk::data::string msg) throw()
	{
		str = msg;
	}

	exception::~exception() throw()
	{
	}

	const char* exception::what() const throw()
	{
		return str.c_str();
	}

	notimplementexception::notimplementexception(lyramilk::data::string msg) throw() : exception(msg)
	{
	}
}