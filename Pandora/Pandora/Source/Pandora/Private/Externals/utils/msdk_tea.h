#ifndef __PDR_MSDK_TEA
#define __PDR_MSDK_TEA

#include "UnrealString.h"

namespace pandora
{
typedef unsigned char msdk_byte;
typedef unsigned int msdk_word32;

int msdk_decode(msdk_byte* encodedDataBytes, int encodedDataBytesLen, msdk_byte* decodedDataBytes, int* decodedStrLen);
TArray<msdk_byte> base64_decode(const TArray<msdk_byte>& encoded_string);
}

#endif