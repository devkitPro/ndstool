#ifndef __CRC32_H
#define __CRC32_H

void crc32Init(unsigned long *pCrc32);
void crc32Update(unsigned long *pCrc32, const unsigned char *pData, unsigned long uSize);
void crc32Finish(unsigned long *pCrc32);

#endif	// __CRC32_H
