#pragma pack(1)

struct Banner
{
	unsigned_short version;
	unsigned_short crc;
	unsigned char reserved[28];
	unsigned char tile_data[4][4][8][4];
	unsigned_short palette[16];
	unsigned_short title[6][128];		// max. 3 lines. seperated by linefeed character
};

#pragma pack()

extern const char *bannerLanguages[];

static inline unsigned int CalcBannerSize(unsigned short version)
{
	switch (version)
	{
		default:     version = 1; /* fallthrough */
		case 0x0001:
		case 0x0002:
		case 0x0003: return 0x840 + 0x100 * (version - 1);
		case 0x0103: return 0x23C0;
	}
};

int InsertTitleString(char *String, FILE *file);
unsigned short CalcBannerCRC(Banner &banner);
void IconFromBMP();
void IconFromGRF();