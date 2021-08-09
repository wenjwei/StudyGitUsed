#include "PImageHelper.h"
#include "PLog.h"

#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
#include "Runtime/ImageWrapper/Public/IImageWrapper.h"
#include "Runtime/ImageWrapper/Public/IImageWrapperModule.h"
#include "Runtime/Engine/Classes/Engine/Texture2D.h"
#include "HAL/UnrealMemory.h"


namespace pandora
{
UTexture2D* PImageHelper::LoadImageToTexture2D(const FString& path, TextureGroup LODGroup, uint8 isSRGB)
{
    UTexture2D * texture = nullptr;
    IImageWrapperModule & module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TArray<uint8> rawdata;
    if (!FFileHelper::LoadFileToArray(rawdata, *path))
    {
        PLog::LogWarning("PImageHelper::LoadImageToTexture2D failed to load file to array");
        return nullptr;
    }

    EImageFormat format = module.DetectImageFormat(rawdata.GetData(), rawdata.Num());
    if (format == EImageFormat::Invalid)
    {
        PLog::LogWarning("PImageHelper::LoadImageToTexture2D format invalid");
        return nullptr;
    }

    TSharedPtr<IImageWrapper> wrapper = module.CreateImageWrapper(format);
    if (!wrapper.IsValid())
    {
        PLog::LogWarning("PImageHelper::LoadImageToTexture2D wrapper invalid");
        return nullptr;
    }

    if (!wrapper->SetCompressed(rawdata.GetData(), rawdata.Num()))
    {
        PLog::LogWarning("PImageHelper::LoadImageToTexture2D wrapper SetCompressed failed");
        return nullptr;
    }

    const TArray<uint8>* uncompressedBGRA = NULL;
    if (!wrapper->GetRaw(ERGBFormat::BGRA, 8, uncompressedBGRA))
    {
        PLog::LogWarning("PImageHelper::LoadImageToTexture2D wrapper GetRaw failed");
        return nullptr;
    }

    texture = UTexture2D::CreateTransient(wrapper->GetWidth(), wrapper->GetHeight(), PF_B8G8R8A8);
    if (!texture)
    {
        PLog::LogWarning("PImageHelper::LoadImageToTexture2D UTexture2D::CreateTransient failed");
        return nullptr;
    }

    void * tdata = texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(tdata, uncompressedBGRA->GetData(), uncompressedBGRA->Num());
    texture->PlatformData->Mips[0].BulkData.Unlock();
    texture->LODGroup = LODGroup;
    texture->SRGB = isSRGB;

    texture->UpdateResource();
    return texture;
}

void PImageHelper::UpdateTextureResources(UTexture2D* texture)
{
    texture->UpdateResource();
}

}
