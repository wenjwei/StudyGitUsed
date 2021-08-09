#pragma once

#include "SharedPointer.h"
#include "Runtime/Launch/Resources/Version.h"

#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
#include "Engine/TextureDefines.h"
#else
#include "Engine/Classes/Engine/TextureDefines.h"
#endif

class IImageWrapper;
class UTexture2D;
class FString;

namespace pandora
{
class PImageHelper
{
public:
    static UTexture2D* LoadImageToTexture2D(const FString& path, TextureGroup LODGroup, uint8 isSRGB);
    static void UpdateTextureResources(UTexture2D* texture);
};
}
