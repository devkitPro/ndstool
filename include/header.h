#pragma pack(1)

struct Header
{
	char title[0xC];
	char gamecode[0x4];
	char makercode[2];
	unsigned char unitcode;
	unsigned char devicetype;						// type of device in the game card
	unsigned char devicecap;						// device capacity (1<<n Mbit)
	unsigned char reserved1[0x9];
	unsigned char romversion;
	unsigned char reserved2;
	unsigned_int arm9_rom_offset;
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
	unsigned_int rom_control_info1;
	unsigned_int rom_control_info2;
	unsigned_int banner_offset;
	unsigned_short secure_area_crc;
	unsigned_short rom_control_info3;
	unsigned_int offset_0x70;						// ?
	unsigned_int offset_0x74;						// ?
	unsigned_int offset_0x78;						// unique ID for homebrew
	unsigned_int offset_0x7C;						// unique ID for homebrew
	unsigned_int application_end_offset;			// rom size
	unsigned_int rom_header_size;
	unsigned_int offset_0x88;						// reserved... ?
	unsigned_int offset_0x8C;
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
	unsigned char logo[156];
	unsigned_short logo_crc;
	unsigned_short header_crc;
	unsigned_int offset_0x160;
	unsigned_int offset_0x164;
	unsigned_int offset_0x168;
	unsigned_int offset_0x16C;
	unsigned char zero[0x90];
};

#pragma pack()


struct Country
{
	char countrycode;
	char *name;
};

struct Maker
{
	char *makercode;
	char *name;
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
