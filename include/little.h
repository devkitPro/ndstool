#pragma pack(1)

struct unsigned_int
{
	unsigned int i;
	#if BYTE_ORDER == BIG_ENDIAN
	#error "big endian!!!"
		operator unsigned int () { return i<<24 | i<<8&0xFF0000 | i>>8&0xFF00 | i>>24; }
		unsigned int & operator = (unsigned int i) { return this->i = i<<24 | i<<8&0xFF0000 | i>>8&0xFF00 | i>>24; }
	#else
		operator unsigned int () { return i; }
		unsigned int & operator = (unsigned int i) { return this->i = i; }
	#endif
	unsigned_int() {}
	unsigned_int(unsigned int i) { *this = i; }
};


struct unsigned_short
{
	unsigned short i;
	#if __BYTE_ORDER == __BIG_ENDIAN
		operator unsigned short () { return i>>8 | i<<8; }
		unsigned short & operator = (unsigned short i) { return this->i = i>>8 | i<<8; }
	#else
		operator unsigned short () { return i; }
		unsigned short & operator = (unsigned short i) { return this->i = i; }
	#endif
	unsigned_short() {}
	unsigned_short(unsigned short i) { *this = i; }
};

struct signed_int
{
	unsigned int i;
	#if BYTE_ORDER == BIG_ENDIAN
	#error "big endian!!!"
		operator signed int () { return i<<24 | i<<8&0xFF0000 | i>>8&0xFF00 | i>>24; }
		signed int & operator = (signed int i) { return (signed int &)this->i = i<<24 | i<<8&0xFF0000 | i>>8&0xFF00 | i>>24; }
	#else
		operator signed int () { return i; }
		signed int & operator = (signed int i) { return (signed int &)this->i = i; }
	#endif
	signed_int() {}
	signed_int(signed int i) { *this = i; }
};

struct signed_short
{
	unsigned short i;
	#if __BYTE_ORDER == __BIG_ENDIAN
		operator signed short () { return i>>8 | i<<8; }
		signed short & operator = (unsigned short i) { return (signed short &)this->i = i>>8 | i<<8; }
	#else
		operator signed short () { return i; }
		signed short & operator = (unsigned short i) { return (signed short &)this->i = i; }
	#endif
	signed_short() {}
	signed_short(signed short i) { *this = i; }
};

#pragma pack()
