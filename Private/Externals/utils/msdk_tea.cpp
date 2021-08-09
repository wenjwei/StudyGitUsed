#include "msdk_tea.h"

#if PLATFORM_WINDOWS
#include <WinSock2.h>
#else
#include <netinet/in.h>
#endif

#include "Array.h"
#include <stdlib.h>

#define MSDK_ROUNDS 16
#define MSDK_LOG_ROUNDS 4
#define MSDK_SALT_LEN 2
#define MSDK_ZERO_LEN 7

namespace pandora
{

static const msdk_word32 msdk_delta = 0x9e3779b9;
static const msdk_byte* msdk_default_key = (msdk_byte*)("msdkmsdkmsdkmsdk");

static const char* base64_chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

static const int base64_chars_len = strlen(base64_chars);

int find_base64_char(const char& c) {
	for (int i = 0; i < base64_chars_len; ++i)
		if (base64_chars[i] == c)
			return i;
	return -1;
}

void w2b(msdk_word32 value, msdk_byte* bytes, int size = sizeof(msdk_word32)) {
	memset(bytes, 0, sizeof(msdk_byte) * size);
	bytes[0] = (msdk_byte)(0xff & value);
	bytes[1] = (msdk_byte)((0xff00 & value) >> 8);
	bytes[2] = (msdk_byte)((0xff0000 & value) >> 16);
	bytes[3] = (msdk_byte)((0xff000000 & value) >> 24);
}

msdk_word32 b2w(const msdk_byte* bytes, int size = sizeof(msdk_word32)) {
	msdk_word32 value = bytes[0] & 0xff;
	value |= ((bytes[1] << 8) & 0xff00);
	value |= ((bytes[2] << 16) & 0xff0000);
	value |= ((bytes[3] << 24) & 0xff000000);
	return value;
}

/*pOutBuffer、pInBuffer均为8byte, pKey为16byte*/
void TeaDecryptECB(const msdk_byte* pInBuf, const msdk_byte* pKey, msdk_byte* pOutBuf) {
	msdk_word32 y, z, sum;
	msdk_word32 k[4];
	int i;

	/*now encrypted buf is TCP/IP-endian;*/
	/*TCP/IP network byte order (which is big-endian).*/
	y = ntohl(b2w(pInBuf));
	z = ntohl(b2w(pInBuf + 4));

	for (i = 0; i < 4; i++) {
		/*key is TCP/IP-endian;*/
		k[i] = ntohl(b2w(pKey + i * 4));
	}

	sum = msdk_delta << MSDK_LOG_ROUNDS;
	for (i = 0; i < MSDK_ROUNDS; i++) {
		z -= ((y << 4) + k[2]) ^ (y + sum) ^ ((y >> 5) + k[3]);
		y -= ((z << 4) + k[0]) ^ (z + sum) ^ ((z >> 5) + k[1]);
		sum -= msdk_delta;
	}

	w2b(htonl(y), pOutBuf);
	w2b(htonl(z), pOutBuf + 4);

	/*now plain-text is TCP/IP-endian;*/
}

/*pKey为16byte*/
/*
输入:pInBuf为密文格式,nInBufLen为pInBuf的长度是8byte的倍数; *pOutBufLen为接收缓冲区的长度
特别注意*pOutBufLen应预置接收缓冲区的长度!
输出:pOutBuf为明文(Body),pOutBufLen为pOutBuf的长度,至少应预留nInBufLen-10;
返回值:如果格式正确返回0;
*/
/*TEA解密算法,CBC模式*/
/*密文格式:PadLen(1byte)+Padding(var,0-7byte)+Salt(2byte)+Body(var byte)+Zero(7byte)*/
int oi_symmetry_decrypt2(const msdk_byte* pInBuf, int nInBufLen, const msdk_byte* pKey, msdk_byte* pOutBuf, int* pOutBufLen) {

	int nPadLen, nPlainLen;
	msdk_byte dest_buf[8], zero_buf[8];
	const msdk_byte *iv_pre_crypt, *iv_cur_crypt;
	int dest_i, i, j;
	int nBufPos;
	nBufPos = 0;

	if ((nInBufLen % 8) || (nInBufLen < 16))
		return -1;

	TeaDecryptECB(pInBuf, pKey, dest_buf);

	nPadLen = dest_buf[0] & 0x7/*只要最低三位*/;

	/*密文格式:PadLen(1byte)+Padding(var,0-7byte)+Salt(2byte)+Body(var byte)+Zero(7byte)*/
	i = nInBufLen - 1/*PadLen(1byte)*/ - nPadLen - MSDK_SALT_LEN - MSDK_ZERO_LEN; /*明文长度*/
	if ((*pOutBufLen < i) || (i < 0))
		return -1;
	*pOutBufLen = i;

	for (i = 0; i < 8; i++)
		zero_buf[i] = 0;

	iv_pre_crypt = zero_buf;
	iv_cur_crypt = pInBuf; /*init iv*/

	pInBuf += 8;
	nBufPos += 8;

	dest_i = 1; /*dest_i指向dest_buf下一个位置*/

	/*把Padding滤掉*/
	dest_i += nPadLen;

	/*dest_i must <=8*/

	/*把Salt滤掉*/
	for (i = 1; i <= MSDK_SALT_LEN;) {
		if (dest_i < 8) {
			dest_i++;
			i++;
		} else if (dest_i == 8) {
			/*解开一个新的加密块*/

			/*改变前一个加密块的指针*/
			iv_pre_crypt = iv_cur_crypt;
			iv_cur_crypt = pInBuf;

			/*异或前一块明文(在dest_buf[]中)*/
			for (j = 0; j < 8; j++) {
				if ((nBufPos + j) >= nInBufLen)
					return -1;
				dest_buf[j] ^= pInBuf[j];
			}

			/*dest_i==8*/
			TeaDecryptECB(dest_buf, pKey, dest_buf);

			/*在取出的时候才异或前一块密文(iv_pre_crypt)*/

			pInBuf += 8;
			nBufPos += 8;

			dest_i = 0; /*dest_i指向dest_buf下一个位置*/
		}
	}

	/*还原明文*/

	nPlainLen = *pOutBufLen;
	while (nPlainLen) {
		if (dest_i < 8) {
			*(pOutBuf++) = dest_buf[dest_i] ^ iv_pre_crypt[dest_i];
			dest_i++;
			nPlainLen--;
		} else if (dest_i == 8) {
			/*dest_i==8*/

			/*改变前一个加密块的指针*/
			iv_pre_crypt = iv_cur_crypt;
			iv_cur_crypt = pInBuf;

			/*解开一个新的加密块*/

			/*异或前一块明文(在dest_buf[]中)*/
			for (j = 0; j < 8; j++) {
				if ((nBufPos + j) >= nInBufLen)
					return -1;
				dest_buf[j] ^= pInBuf[j];
			}

			TeaDecryptECB(dest_buf, pKey, dest_buf);

			/*在取出的时候才异或前一块密文(iv_pre_crypt)*/

			pInBuf += 8;
			nBufPos += 8;

			dest_i = 0; /*dest_i指向dest_buf下一个位置*/
		}
	}

	/*校验Zero*/
	for (i = 1; i <= MSDK_ZERO_LEN;) {
		if (dest_i < 8) {
			if (dest_buf[dest_i] ^ iv_pre_crypt[dest_i])
				return -1;
			dest_i++;
			i++;
		} else if (dest_i == 8) {
			/*改变前一个加密块的指针*/
			iv_pre_crypt = iv_cur_crypt;
			iv_cur_crypt = pInBuf;

			/*解开一个新的加密块*/
			/*异或前一块明文(在dest_buf[]中)*/
			for (j = 0; j < 8; j++) {
				if ((nBufPos + j) >= nInBufLen)
					return -1;
				dest_buf[j] ^= pInBuf[j];
			}

			TeaDecryptECB(dest_buf, pKey, dest_buf);

			/*在取出的时候才异或前一块密文(iv_pre_crypt)*/
			pInBuf += 8;
			nBufPos += 8;
			dest_i = 0; /*dest_i指向dest_buf下一个位置*/
		}

	}

	return 0;
}

static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

TArray<msdk_byte> base64_decode(const TArray<msdk_byte>& encoded_string) {
	int in_len = encoded_string.Num();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	TArray<msdk_byte> ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_];
		in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++)
				char_array_4[i] = find_base64_char(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret.Add(char_array_3[i]);
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = find_base64_char(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
			ret.Add(char_array_3[j]);
	}

	return ret;
}

int msdk_decode(msdk_byte* encodedDataBytes, int encodedDataBytesLen, msdk_byte* decodedDataBytes, int* decodedStrLen) {
	return oi_symmetry_decrypt2(encodedDataBytes, encodedDataBytesLen, msdk_default_key, decodedDataBytes, decodedStrLen);
}

}
