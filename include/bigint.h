struct BigInt
{
	unsigned char data[129*2];

	int Sub(BigInt &a, BigInt &b, int b_shift)
	{
		int b_byte = - (b_shift / 8);
		int b_bit = (b_shift % 8);
		int carry = 0;
		for (unsigned int ia=0; ia<sizeof(data); ia++)
		{
			unsigned char b_data_shifted = 0;
			if (b_byte - 1 >= 0) b_data_shifted |= b.data[b_byte - 1] >> (8 - b_bit);
			if (b_byte + 0 >= 0) b_data_shifted |= b.data[b_byte + 0] << b_bit;
			b_byte++;
			int r = a.data[ia] - b_data_shifted + carry;
			data[ia] = r;
			carry = r>>8;
		}
		return carry;
	}

	void MulMod(BigInt &a, BigInt &b, BigInt &m)
	{
		memset(this, 0, sizeof(*this));
		for (int ia=0; ia<sizeof(a.data)/2; ia++)
		{
			int carry = 0;
			for (int ib=0; ib<sizeof(b.data)/2; ib++)
			{
				unsigned int r = a.data[ia] * b.data[ib] + carry + data[ia+ib];
				data[ia+ib] = r;
				carry = r>>8;
			}
		}

		for (int shift=sizeof(data)/2*8; shift>=0; shift--)
		{
			BigInt subbed;
			if (!subbed.Sub(*this, m, shift))	// positive?
			{
				memcpy(this, &subbed, sizeof(*this));
			}
		}
	}

	void PowMod(BigInt &n, BigInt &m)
	{
		BigInt n65536;
		memcpy(&n65536, &n, sizeof(n));
		for (int i=0; i<16; i++)
		{
			BigInt tmp;
			tmp.MulMod(n65536, n65536, m);
			memcpy(&n65536, &tmp, sizeof(n));
		}
		MulMod(n, n65536, m);	// n^1 * n^65536
	}

	void print()
	{
		printf("[ 0x");
		bool show = false;
		for (int i=sizeof(data)-1; i>=0; i--)
		{
			if (data[i]) show = true;
			if (show) printf("%02X", data[i]);
		}
		printf(" ]\n");
	}
	
	void Set(unsigned char *data, int length)
	{
		memset(this, 0, sizeof(*this));
		for (int i=0; i<length; i++)
		{
			this->data[length - 1 - i] = data[i];
		}
	}

	void Get(unsigned char *data, int length)
	{
		for (int i=0; i<length; i++)
		{
			data[i] = this->data[length - 1 - i];
		}
	}
};
