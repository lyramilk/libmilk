#include "hash.h"
namespace lyramilk{ namespace cryptology
{
	namespace hash32{
		unsigned int bkdr(const char* p,std::size_t l)
		{
			unsigned int r = 0;
			for(;l > 0;--l){
				r = r*131 + (*p++);
			}
			return r;
		}

		unsigned int djb(const char* p,std::size_t l)
		{
			unsigned int r = 5381;
			for(;l > 0;--l){
				r = (r << 5) + r + (*p++);
			}
			return r;
		}

		unsigned int ap(const char* p,std::size_t l)
		{
			unsigned int r = 0;
			for(;l > 0;--l){
				if(r&1){
					r ^= (r << 7) ^ (*p++) ^ (r >> 3);
				}else{
					r ^= ~((r << 11) ^ (*p++) ^ (r >> 5));
				}
			}
			return r;
		}

		unsigned int dek(const char* p,std::size_t l)
		{
			unsigned int r = 1315423911;
			for(;l > 0;--l){
				r = (r << 5) ^ (r >> 27) ^ (*p++);
			}
			return r;
		}

		unsigned int murmur2(const char* p,std::size_t l)
		{
			const static unsigned int seed = 0xee6b27eb;
			const static unsigned int m = 0x5bd1e995;
			const static int r = 24;

			unsigned int h = seed ^ l;

			while(l >= 4)
			{
				unsigned int k = *(unsigned int*)p;

				k *= m; 
				k ^= k >> r; 
				k *= m; 

				h *= m;
				h ^= k;

				p += 4;
				l -= 4;
			}

			switch(l) {
			  case 3: h ^= p[2] << 16;
			  case 2: h ^= p[1] << 8;
			  case 1: h ^= p[0];
				h *= m;
			};

			h ^= h >> 13;
			h *= m;
			h ^= h >> 15;
			return h;
		}

		unsigned int fnv(const char* p,std::size_t l)
		{
			unsigned int r = 2166136261;
			for(;l > 0;--l){
				r ^=  (unsigned int)(*p++);
				r *= 16777619;
			}
			return r;
		}
	}

	namespace hash64{
		unsigned long long murmur2(const char* data,std::size_t l)
		{
			const static unsigned long long seed = 0xee6b27eb;
			const static unsigned long long m = 0xc6a4a7935bd1e995ULL;
			const static int r = 47;

			const unsigned long long* p = (const unsigned long long*)data;

			const unsigned long long* end = p + (l/8);
			unsigned long long h = seed ^ (l * m);

			while(p != end)
			{
				unsigned long long k = *p++;

				k *= m; 
				k ^= k >> r; 
				k *= m; 

				h ^= k;
				h *= m; 
			}

			const unsigned char * p2 = (const unsigned char*)p;

			switch(l & 7)
			{
			  case 7: h ^= (unsigned long long)(p2[6]) << 48;
			  case 6: h ^= (unsigned long long)(p2[5]) << 40;
			  case 5: h ^= (unsigned long long)(p2[4]) << 32;
			  case 4: h ^= (unsigned long long)(p2[3]) << 24;
			  case 3: h ^= (unsigned long long)(p2[2]) << 16;
			  case 2: h ^= (unsigned long long)(p2[1]) << 8;
			  case 1: h ^= (unsigned long long)(p2[0]);
				h *= m;
			};

			h ^= h >> r;
			h *= m;
			h ^= h >> r;

			return h;
		}

		unsigned long long fnv(const char* p,std::size_t l)
		{
			unsigned long long r = 14695981039346656037ULL;
			for(;l > 0;--l){
				r ^=  (unsigned long long)(*p++);
				r *= 1099511628211ULL;
			}
			return r;
		}

		unsigned long long fnvX(const char* data,std::size_t l)
		{
			const unsigned long long* p = (const unsigned long long*)data;
			unsigned long long r = 14695981039346656037ULL;
			for(;l >=8;l-= 8){
				r ^=  (*p++);
				r *= 1099511628211ULL;
			}
			const char* p2 = (const char*)p;
			for(;l > 0;--l){
				r ^=  (std::size_t)(*p2++);
				r *= 1099511628211ULL;
			}
			return r;
		}
	}
}}
