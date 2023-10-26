#include "bigint.h"
#include <sstream>
#include <assert.h>
#include <memory.h>
#include <iomanip>

using namespace lyramilk::data;

bigint::bitset::bitset(bigint& t,unsigned int& o,unsigned int pos)
	:x(t),i(o),p(pos)
{
	assert(p < 32);
}

bigint::bitset& bigint::bitset::operator=(bool v)
{
	if(v){
		i |= 1<<p;
	}else{
		i &= ~(1<<p);
	}
	x.trim();
	return (*this);
}

bigint::bitset& bigint::bitset::operator=(const bigint::bitset& b)
{
	return (*this) = (bool)b;
}

bool bigint::bitset::operator~() const
{
	bool r = !*this;
	x.trim();
	return r;
}

bigint::bitset::operator bool() const
{
	return (i >> p) &1;
}

bool bigint::minus() const
{
	return (d.empty()?0:((d.back()&0x80000000) == 0x80000000));
}

void bigint::bits(bindex_type bitcount)
{
	bindex_type bytes = (bitcount>>5)+((bitcount&31) != 0);

	unsigned int comp = minus()?0xffffffff:0;

	if(d.empty()){
		d.assign(bytes,comp);
	}else if(bytes > d.size()){
		bytes_type v;
		d.insert(d.end(),bytes - d.size(),comp);
	}else if(bytes < d.size()){
		bytes_type v(d.begin(),d.begin() + bytes);
		d.swap(v);
	}
}

std::size_t bigint::bits() const
{
	return d.size() << 5;
}

void bigint::trim()
{
	unsigned int comp = minus()?0xffffffff:0;
	d.push_back(comp);
	while((d.size() > 1) && (d.back() == comp) && *(d.end()-2) == comp) d.pop_back();
}

int bigint::cmp(const bytes_type& o) const
{
	size_t sz_this = d.size();
	size_t sz_o = o.size();
	
	int des = (int)(sz_this - sz_o);

	if(des >= 0){
		bytes_type::const_reverse_iterator rd = d.rbegin();
		for(;des > 0 && rd != d.rend();--des,++rd){
			//当this.d > o.d时，结果与符号相反
			if((*rd) > 0) return 1;
		}
		bytes_type::const_reverse_iterator ro = o.rbegin();
		for(;rd!=d.rend();++rd,++ro)
		{
			if((*rd) > (*ro)) return 1;
			if((*rd) < (*ro)) return -1;
		}
	}else if(des < 0){
		bytes_type::const_reverse_iterator ro = o.rbegin();
		for(;des < 0 && ro != o.rend();++des,++ro){
			//当this.d > o.d时，结果与符号相反
			if((*ro) > 0) return -1;
		}
		bytes_type::const_reverse_iterator rd = d.rbegin();
		for(;rd!=d.rend();++rd,++ro)
		{
			if((*ro) > (*rd)) return -1;
			if((*rd) < (*ro)) return 1;
		}
	}
	return 0;
}


int bigint::cmp(const bigint& o) const
{
	if(minus() > o.minus()) return -1;
	if(minus() < o.minus()) return 1;
	return minus()?(0 - cmp(o.d)):cmp(o.d);
}

bigint bigint::opposite() const
{
	bigint p = *this;
	p.bits(p.bits() + 1);
	p = ~p;
	p += 1;
	return p;
}

bigint::bigint()
{
}

bigint::bigint(int o)
{
	operator =(o);
}

bigint::bigint(long o)
{
	operator =(o);
}

bigint::bigint(long long o)
{
	operator =(o);
}

bigint::bigint(unsigned int o)
{
	operator =(o);
}

bigint::bigint(unsigned long o)
{
	operator =(o);
}

bigint::bigint(unsigned long long o)
{
	operator =(o);
}

bigint::bigint(const bigint& o)
{
	operator =(o);
}

bigint& bigint::operator =(int o)
{
	d.clear();
	d.push_back(o);
	trim();
	return *this;
}

bigint& bigint::operator =(long o)
{
	return *this = (long long)o;
}

bigint& bigint::operator =(long long o)
{
	d.clear();
	union
	{
		unsigned long long p;
		unsigned int b[1];
	} u;
	u.p = o;

	for(int i=0;i<2;++i){
		d.push_back(u.b[i]);
	}
	trim();
	return *this;
}

bigint& bigint::operator =(unsigned int o)
{
	d.clear();
	d.push_back(o);
	d.push_back(0);
	trim();
	return *this;
}

bigint& bigint::operator =(unsigned long o)
{
	return *this = (unsigned long long)o;
}

bigint& bigint::operator =(unsigned long long o)
{
	d.clear();
	union
	{
		unsigned long long p;
		unsigned int b[1];
	} u;
	u.p = o;

	for(int i=0;i<2;++i){
		d.push_back(u.b[i]);
	}
	d.push_back(0);
	trim();
	return *this;
}

bigint& bigint::operator =(const bigint& o)
{
	d = o.d;
	return *this;
}

bigint::bitset bigint::operator[](bindex_type o)
{
	if(bits() < (o + 2)) bits(o + 2);
	bindex_type bytes = o>>5;

	return bitset(*this,d.at(bytes),o&31);
}

bool bigint::operator < (const bigint& o) const
{
	return cmp(o) < 0;
}

bool bigint::operator > (const bigint& o) const
{
	return cmp(o) > 0;
}

bool bigint::operator <= (const bigint& o) const
{
	return cmp(o) != 1;
}

bool bigint::operator >= (const bigint& o) const
{
	return cmp(o) != -1;
}

bool bigint::operator == (const bigint& o) const
{
	return cmp(o) == 0;
}

bool bigint::operator != (const bigint& o) const
{
	return cmp(o) != 0;
}

bigint& bigint::operator += (const bigint& o)
{
	unsigned long long cr = 0;
	bool sf = minus();
	//保证this的位数不小于o
	if(d.size() < o.d.size()){
		bits(o.bits());
	}

	bytes_type::const_iterator io = o.d.begin();
	bytes_type::iterator id = d.begin();

	for(;io != o.d.end();++id,++io){
		unsigned long long sum = ((unsigned long long)(*io)) + *id + cr;
		cr = sum >> 32;
		*id = sum&0xffffffff;
	}
	for(;id != d.end();++id){
		
		unsigned long long sum = ((unsigned long long)(*id)) + cr + (o.minus()?0xffffffff:0);
		cr = sum >> 32;
		*id = sum&0xffffffff;
	}


	if(cr > 0){
		//溢出后如果同号则真正是溢出
		if(sf == o.minus()){
			if(sf){
				d.push_back(0xffffffff);
			}else{
				d.push_back((unsigned int)cr);
				d.push_back(0);
			}
		}
	}
	trim();
	return *this;
}

bigint bigint::operator + (const bigint& o) const
{
	return bigint(*this) += o;
}

bigint& bigint::operator -= (const bigint& o)
{
	return *this += o.opposite();
}

bigint bigint::operator - (const bigint& o) const
{
	return bigint(*this) -= o;
}

bigint& bigint::operator *= (const bigint& o)
{
	bigint x = minus()?opposite():*this;
	bigint y = o.minus()?o.opposite():o;

	bindex_type i = x.bits();
	x.bits(i + 1);
	
	bindex_type b = 0;

	bigint result = 0;
	for(;x != 0;x<<=1,++b){
		if(x[i] == 1) break;
	}

	for(;b <= i;x<<=1,++b){
		result <<= 1;
		if(x[i] == 1){
			result += y;
		}
	}

	return *this = (o.minus() == minus())?result:result.opposite();
}

bigint bigint::operator * (const bigint& o) const
{
	return bigint(*this) *= o;
}

bigint& bigint::operator /= (const bigint& o)
{
	bigint div = o.minus()?o.opposite():o;
	bigint odiv = div;
	bigint x = minus()?opposite():*this;
	
	bigint result = 0;

	while(div <= x) div<<= 1;

	while(div!=odiv){
		div >>= 1;
		result<<=1;
		if(x >= div){
			x -= div;
			++result;
		}
	}

	return *this = (o.minus() == minus())?result:result.opposite();
}

bigint bigint::operator / (const bigint& o) const
{
	return bigint(*this) /= o;
}

bigint& bigint::operator %= (const bigint& o)
{
	bool sfx = minus();
	bool sfy = o.minus();

	bigint div = sfy?o.opposite():o;
	bigint odiv = div;
	bigint x = sfx?opposite():*this;
	
	while(div <= x) div<<= 1;

	while(div!=odiv){
		div >>= 1;
		if(x >= div){
			x -= div;
		}
	}
	if(sfx == sfy){
		*this = sfx?x.opposite():x;
	}else if(sfy){
		*this = x + o;
	}else if(sfx){
		*this = sfx?x.opposite():x;
	}
	return *this;
}

bigint bigint::operator % (const bigint& o) const
{
	return bigint(*this) %= o;
}

bigint& bigint::operator ++()
{
	return *this += 1;
}

bigint bigint::operator ++(int)
{
	bigint o = *this;
	++*this;
	return o;
}


bigint& bigint::operator --()
{
	return *this -= 1;
}

bigint bigint::operator --(int)
{
	bigint o = *this;
	--*this;
	return o;
}

bigint bigint::operator ~() const
{
	bigint p;
	
	for(bytes_type::const_iterator it = d.begin();it!=d.end();++it)
	{
		p.d.push_back(~*it);
	}
	//p.trim();
	return p;
}

bool bigint::operator !() const
{
	return *this == 0;
}

bigint& bigint::operator ^= (const bigint& o)
{
	bigint k = o;
	if(bits() > k.bits()){
		k.bits(bits());
	}else{
		bits(k.bits());
	}

	bytes_type::iterator itx = d.begin();
	bytes_type::iterator ity = k.d.begin();

	for(;itx != d.end() && ity != k.d.end();++itx,++ity){
		*itx ^= *ity;
	}
	trim();
	return *this;
}

bigint bigint::operator ^ (const bigint& o) const
{
	return bigint(*this) ^= o;
}

bigint& bigint::operator &= (const bigint& o)
{
	bigint k = o;
	if(bits() > k.bits()){
		k.bits(bits());
	}else{
		bits(k.bits());
	}

	bytes_type::iterator itx = d.begin();
	bytes_type::iterator ity = k.d.begin();

	for(;itx != d.end() && ity != k.d.end();++itx,++ity){
		*itx &= *ity;
	}
	trim();
	return *this;
}

bigint bigint::operator & (const bigint& o) const
{
	return bigint(*this) &= o;
}

bigint& bigint::operator <<= (int o)
{
	bindex_type b = bits();
	bits(o + (d.size()<<5) + 1);
	if (o > 31)
	{
		int c = o/32;

		bytes_type::reverse_iterator it = d.rbegin();
		for(;it != d.rend() - c;++it){
			*it = *(it + c);
		}
		memset(d.data(),0,c);
	}
	unsigned long long cr = 0;
	unsigned int md = o%32;

	bytes_type::iterator it = d.begin();
	for(;b >= 0 && it != d.end();++it,--b){
		unsigned long long sum = (((unsigned long long)(unsigned int)*it) << md)|cr;
		cr = sum >> 32;
		*it = sum&0xffffffff;
	}
	trim();
	return *this;
}

bigint bigint::operator << (int o) const
{
	return bigint(*this) <<= o;
}

bigint& bigint::operator >>= (int o)
{
	if (o > 31)
	{
		int c = o/32;

		bytes_type::iterator it = d.begin();
		for(;it != d.end() - c;++it){
			*it = *(it + c);
		}
		for(;it != d.end();++it){
			*it = 0;
		}
	}
	unsigned long long cr = 0;
	unsigned int md = o%32;
	unsigned long long mask = (1<<md) - 1;

	bytes_type::reverse_iterator it = d.rbegin();
	for(;it != d.rend();++it){
		unsigned long long i = ((unsigned long long)(unsigned int)*it) | (cr << 32);
		cr = i & mask;
		*it = (unsigned int)(i>>md);
	}
	trim();
	return *this;
}

bigint bigint::operator >> (int o) const
{
	return bigint(*this) >>= o;
}


bigint bigint::pow(const bigint& _x,const bigint& _n)
{
	assert(!_n.minus());
	bigint x = _x;
	bigint n = _n;
	for(bigint z=1;;x*=x){
		if((n&1) != 0){
			z*=x;
		}
		if((n>>=1)==0) return z;
	}
}

bigint bigint::powmod(const bigint& _x,const bigint& _n,const bigint& m)
{
	assert(!_n.minus());
	bigint x = _x;
	bigint n = _n;
	for(bigint z=1;;(x*=x)%=m){
		if((n&1) != 0){
			(z*=x)%=m;
		}
		if((n>>=1)==0) return z;
	}
}

/////////////////////////表现

std::ostream& lyramilk::data::operator << (std::ostream& o,const bigint& r)
{
	std::ostream::fmtflags fg = o.flags();
	if(fg & o.hex){
		o << "0x";
		bigint::bytes_type::const_reverse_iterator it = r.d.rbegin();
		for(;it!=r.d.rend() && *it == 0;++it);
		for(;it!=r.d.rend();++it){
			o << (int)*it;
		}
	}else{
		//十进制
		//快除
		std::stringstream ss;
		bool sf = r.minus();
		bigint x = sf?r.opposite():r;
		bigint div = 1000000000;
		bigint result = 0;
		std::vector<bigint> tmp;
		std::vector<bigint> blocks;
		while(div <= x){
			tmp.push_back(div);
			div *= div;
		}
		blocks.push_back(x);
		for(std::vector<bigint>::reverse_iterator it = tmp.rbegin();it != tmp.rend();++it){
			std::vector<bigint> bt;
			bigint& i = *it;
			for(std::vector<bigint>::reverse_iterator it2 = blocks.rbegin();it2 != blocks.rend();++it2){
				bigint& k = *it2;
				while(k != 0){
					bigint m = k%i;
					k /= i;
					bt.push_back(m);
				}
			}
			blocks.clear();
			blocks.assign(bt.rbegin(),bt.rend());
		}

		
		for(std::vector<bigint>::iterator it3 = blocks.begin();it3 != blocks.end();++it3){
			ss << std::setfill('0') << std::setw(9) << it3->d.front();
		}
		if(sf) o.put('-');

		std::string st = ss.str();
		std::string::iterator it = st.begin();
		for(;it != st.end() && *it == '0';++it);
		for(;it != st.end();++it){
			o.put(*it);
		}
	}

	return o;
}

std::istream& lyramilk::data::operator >> (std::istream& o,bigint& r)
{
	assert(r != r);
	return o;
}