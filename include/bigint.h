struct BigInt
{
	unsigned char data[129*2];

	int Sub(BigInt &a, BigInt &b, int b_shift);
	void MulMod(BigInt &a, BigInt &b, BigInt &m);
	void PowMod(BigInt &n, BigInt &m);
	void print();
	void Set(unsigned char *data, int length);
	void Get(unsigned char *data, int length);
};
