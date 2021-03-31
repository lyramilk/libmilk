#ifndef _lyramilk_bloomfilter_h_
#define _lyramilk_bloomfilter_h_
#include <vector>
#include <string>

/// namespace lyramilk::data
namespace lyramilk{ namespace data
{
	/*
		布隆过滤器
	*/
	class bloomfilter
	{
		std::vector<void*> bfarray;
		unsigned int bit_per_key;
		unsigned int total_bits;

		unsigned long long data_count;

		const char* data();
		unsigned int size();
	 public:
		/*
			@param expect_count	预计存入的键数
			@param bit_per_key	每个键用的哈希次数
		*/
		bloomfilter(unsigned int expect_count,unsigned int bit_per_key = 6);
		virtual ~bloomfilter();

		virtual void put(const char* key,unsigned int len);
		virtual bool test(const char* key,unsigned int len) const;
		/*
			@brief 误判率
			@param fast	为true时是用公式计算，为false时会扫描布隆过滤器的实际状态然后计算
		*/
		double false_positives(bool fast = true);

	  public:
		bool dump(std::string* arg);
		bool from(const std::string& arg);
	};

}}

#endif
