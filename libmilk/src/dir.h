#ifndef _lyramilk_data_dir_h_
#define _lyramilk_data_dir_h_
#include <vector>
#include <dirent.h>
#include "def.h"

namespace lyramilk{ namespace io
{
	typedef bool (*scan_dir_callback)(struct dirent *ent,const lyramilk::data::string& filename,void*userdata);
	bool scan_dir(const char* pathdirname,scan_dir_callback cbk,void *userdata = nullptr,bool recursion = false);

}}

#endif
