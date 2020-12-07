#include "dir.h"
#include "def.h"
#include <sys/stat.h>

namespace lyramilk{ namespace io
{

	struct dircls
	{
		DIR* d;
		dircls(const char* pathdirname)
		{
			d = opendir(pathdirname);
		}

		~dircls()
		{
			if(d){
				closedir(d);
			}
		}
	};




	bool static scan_dir_internal(const lyramilk::data::string& pathdirname,scan_dir_callback cbk,void *userdata,bool recursion)
	{
		lyramilk::data::string filename = pathdirname;
		filename.push_back('/');
		dircls dirobj(pathdirname.c_str());
		if(dirobj.d){
			struct dirent ent;
			struct dirent* pent;
			while(readdir_r(dirobj.d,&ent,&pent)==0 && pent != nullptr){
				if(ent.d_name[0] == '.'){
					if(ent.d_name[1] == '.'){
						if(ent.d_name[2] == 0) continue;
					}else if(ent.d_name[1] == 0){
						continue;
					}
				}

				lyramilk::data::string curr = filename + ent.d_name;
				if(recursion){
					if(ent.d_type&DT_DIR){
						if(!scan_dir_internal(curr,cbk,userdata,recursion)){
							return false;
						}
					}
				}

				if(cbk){
					if(!cbk(pent,curr,userdata)){
						return false;
					}
				}
			}
			return true;
		}
		return false;
	}

	bool scan_dir(const char* pathdirname,scan_dir_callback cbk,void *userdata,bool recursion)
	{
		lyramilk::data::string pathfile = pathdirname;
		std::size_t p = pathfile.find_last_not_of("/");
		if(p!=pathfile.npos){
			pathfile = pathfile.substr(0,p + 1);
		}
		struct stat st = {0};
		if(0 !=::stat(pathfile.c_str(),&st)){
			return false;
		}
		return scan_dir_internal(pathfile,cbk,userdata,recursion);
	}
}}
