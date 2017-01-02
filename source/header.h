#pragma pack(1)

struct Header
{
	char title[0xC];
	char gamecode[0x4];
	char makercode[2];
	unsigned char unitcode;							// product code. 0=NDS, 2=NDS+DSi, 3=DSi
	unsigned char devicetype;						// device code. 0 = normal
	unsigned char devicecap;						// device size. (1<<n Mbit)
	unsigned char reserved1[0x7];					// 0x015..0x01D
	unsigned char dsi_flags;
	unsigned char nds_region;
	unsigned char romversion;
	unsigned char reserved2;						// 0x01F
	unsigned_int arm9_rom_offset;					// points to libsyscall and rest of ARM9 binary
	unsigned_int arm9_entry_address;
	unsigned_int arm9_ram_address;
	unsigned_int arm9_size;
	unsigned_int arm7_rom_offset;
	unsigned_int arm7_entry_address;
	unsigned_int arm7_ram_address;
	unsigned_int arm7_size;
	unsigned_int fnt_offset;
	unsigned_int fnt_size;
	unsigned_int fat_offset;
	unsigned_int fat_size;
	unsigned_int arm9_overlay_offset;
	unsigned_int arm9_overlay_size;
	unsigned_int arm7_overlay_offset;
	unsigned_int arm7_overlay_size;
	unsigned_int rom_control_info1;					// 0x00416657 for OneTimePROM
	unsigned_int rom_control_info2;					// 0x081808F8 for OneTimePROM
	unsigned_int banner_offset;
	unsigned_short secure_area_crc;
	unsigned_short rom_control_info3;				// 0x0D7E for OneTimePROM
	unsigned_int offset_0x70;						// magic1 (64 bit encrypted magic code to disable LFSR)
	unsigned_int offset_0x74;						// magic2
	unsigned_int offset_0x78;						// unique ID for homebrew
	unsigned_int offset_0x7C;						// unique ID for homebrew
	unsigned_int application_end_offset;			// rom size
	unsigned_int rom_header_size;
	unsigned_int offset_0x88;						// reserved... ?
	unsigned_int offset_0x8C;

	// reserved
	unsigned_int offset_0x90;
	unsigned_int offset_0x94;
	unsigned_int offset_0x98;
	unsigned_int offset_0x9C;
	unsigned_int offset_0xA0;
	unsigned_int offset_0xA4;
	unsigned_int offset_0xA8;
	unsigned_int offset_0xAC;
	unsigned_int offset_0xB0;
	unsigned_int offset_0xB4;
	unsigned_int offset_0xB8;
	unsigned_int offset_0xBC;

	unsigned char logo[156];						// character data
	unsigned_short logo_crc;
	unsigned_short header_crc;

	// 0x160..0x17F reserved
	unsigned_int debug_rom_offset;
	unsigned_int debug_size;
	unsigned_int debug_ram_address;
	unsigned_int offset_0x16C;
	unsigned char zero[0x10];

	// DSi extended stuff below
	u8 global_mbk_setting[5][4];
	unsigned_int arm9_mbk_setting[3];
	unsigned_int arm7_mbk_setting[3];
	unsigned_int mbk9_wramcnt_setting;

	unsigned_int region_flags;
	unsigned_int access_control;
	unsigned_int scfg_ext_mask;
	u8 offset_0x1BC[3];
	u8 appflags;

	unsigned_int dsi9_rom_offset;
	unsigned_int offset_0x1C4;
	unsigned_int dsi9_ram_address;
	unsigned_int dsi9_size;
	unsigned_int dsi7_rom_offset;
	unsigned_int offset_0x1D4;
	unsigned_int dsi7_ram_address;
	unsigned_int dsi7_size;

	unsigned_int digest_ntr_start;
	unsigned_int digest_ntr_size;
	unsigned_int digest_twl_start;
	unsigned_int digest_twl_size;
	unsigned_int sector_hashtable_start;
	unsigned_int sector_hashtable_size;
	unsigned_int block_hashtable_start;
	unsigned_int block_hashtable_size;
	unsigned_int digest_sector_size;
	unsigned_int digest_block_sectorcount;

	unsigned_int banner_size;
	unsigned_int offset_0x20C;
	unsigned_int total_rom_size;
	unsigned_int offset_0x214;
	unsigned_int offset_0x218;
	unsigned_int offset_0x21C;

	unsigned_int modcrypt1_start;
	unsigned_int modcrypt1_size;
	unsigned_int modcrypt2_start;
	unsigned_int modcrypt2_size;

	unsigned_int tid_low;
	unsigned_int tid_high;
	unsigned_int public_sav_size;
	unsigned_int private_sav_size;
	u8 reserved3[176];
	u8 age_ratings[0x10];

	u8 hmac_arm9[20];
	u8 hmac_arm7[20];
	u8 hmac_digest_master[20];
	u8 hmac_icon_title[20];
	u8 hmac_arm9i[20];
	u8 hmac_arm7i[20];
	u8 reserved4[40];
	u8 hmac_arm9_no_secure[20];
	u8 reserved5[2636];
	u8 debug_args[0x180];
	u8 rsa_signature[0x80];
};

#pragma pack()


struct Country
{
	const char countrycode;
	const char *name;
};

struct Maker
{
	const char *makercode;
	const char *name;
};

extern Country countries[];
extern int NumCountries;

extern Maker makers[];
extern int NumMakers;

unsigned short CalcHeaderCRC(Header &header);
unsigned short CalcLogoCRC(Header &header);
void FixHeaderCRC(char *ndsfilename);
void ShowInfo(char *ndsfilename);
int HashAndCompareWithList(char *filename, unsigned char sha1[]);
int DetectRomType();
unsigned short CalcSecureAreaCRC(bool encrypt);
