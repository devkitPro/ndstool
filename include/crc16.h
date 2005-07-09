#ifndef _CRC16_H
#define _CRC16_H

void crc16_init(unsigned short *uCrc16);
void crc16_update(unsigned short *uCrc16, const unsigned char *pBuffer, unsigned long uBufSize);
void crc16_final(unsigned short *uCrc16);

unsigned short CalcCRC(unsigned char *data, unsigned int length);	// without final

#endif	// _CRC16_H
