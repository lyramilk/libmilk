#include "bloomfilter.h"
#include "hash.h"
#include <math.h>
#include <stdlib.h>

namespace lyramilk{ namespace data
{

	template <int T>
	struct tempc;


	template <>
	struct tempc<4>
	{
		enum {
			square = 5,
			bites = 32,
		};
		typedef unsigned int inttype;
	};


	template <>
	struct tempc<8>
	{
		enum {
			square = 6,
			bites = 64,
		};
		typedef unsigned long long inttype;
	};


	bloomfilter::bloomfilter(unsigned int expect_count,unsigned int bit_per_key)
	{
		data_count = 0;
		this->bit_per_key = bit_per_key;
		unsigned int totalbits = (expect_count * bit_per_key * 1.442695040889);

		unsigned int totalbytes = (totalbits >> tempc<sizeof(void*)>::square);
		unsigned int totalbytes1 = (totalbits&(tempc<sizeof(void*)>::bites - 1)) != 0;


		bfarray.resize(totalbytes + totalbytes1);
		this->total_bits = bfarray.size() << tempc<sizeof(void*)>::square;
	}

	bloomfilter::~bloomfilter()
	{
	}


	inline unsigned int rehash(unsigned int h,unsigned int x,unsigned int y,unsigned int z)
	{
		return h * 131 + x;
	}



	void bloomfilter::put(const char* key,unsigned int len)
	{
		++data_count;

		unsigned int h = lyramilk::cryptology::hash32::murmur2(key,len);
		unsigned int x = ((h >> 17) | (h << 15));
		unsigned int y = ((h << 5) ^  (h >> 27));
		unsigned int z = 0xc6a4a793;

		for(unsigned int i=0;i<bit_per_key;++i){
			const tempc<sizeof(void*)>::inttype bitpos = h % total_bits;
			const tempc<sizeof(void*)>::inttype major = bitpos >> tempc<sizeof(void*)>::square;
			const tempc<sizeof(void*)>::inttype minor = bitpos & (tempc<sizeof(void*)>::bites - 1);
			tempc<sizeof(void*)>::inttype& t = (tempc<sizeof(void*)>::inttype&)bfarray[major];
			t |= (1UL << minor);

			h = rehash(h,x,y,z);
		}
	}

	bool bloomfilter::test(const char* key,unsigned int len) const
	{
		unsigned int h = lyramilk::cryptology::hash32::murmur2(key,len);
		unsigned int x = ((h >> 17) | (h << 15));
		unsigned int y = ((h << 5) ^  (h >> 27));
		unsigned int z = 0xc6a4a793;

		for(unsigned int i=0;i<bit_per_key;++i){
			const tempc<sizeof(void*)>::inttype bitpos = h % total_bits;
			const tempc<sizeof(void*)>::inttype major = bitpos >> tempc<sizeof(void*)>::square;
			const tempc<sizeof(void*)>::inttype minor = bitpos & (tempc<sizeof(void*)>::bites - 1);
			tempc<sizeof(void*)>::inttype& t = (tempc<sizeof(void*)>::inttype&)bfarray[major];
			if((t & (1UL << minor)) == 0) return false;

			h = rehash(h,x,y,z);
		}
		return true;
	}

	const char* bloomfilter::data()
	{
		return (const char*)bfarray.data();
	}

	unsigned int bloomfilter::size()
	{
		return bfarray.size() << (tempc<sizeof(void*)>::square - 3);
	}

	double bloomfilter::false_positives(bool fast)
	{
		double k = bit_per_key;

		if(fast){
			double m = total_bits;
			double n = data_count;

			double f1 = 1/m;
			double f2 = 1-f1;
			double f3 = pow(f2,k*n);
			double f4 = 1-f3;
			double f5 = pow(f4,k);

			return f5;

		}

		const char* data = this->data();
		unsigned long long payload = 0;
		unsigned long long empty = 0;
		for(unsigned long long i=0;i<size();++i){
			char c = data[i];
			for(int h=0;h<8;++h,c>>=1){
				if(c&1){
					++payload;
				}else{
					++empty;
				}
			}
		}

		double real_f4 = double(payload)/double(total_bits);
		return pow(real_f4,k);
	}

}}

