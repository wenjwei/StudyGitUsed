#include "PCppInterface.h"
#include "PPandora.h"
#include "PLog.h"
#include "PCppUtilities.h"
#include "PImageHelper.h"

#include "Runtime/UMG/Public/Animation/WidgetAnimation.h"
#include "UMG.h"
#include "TimerManager.h"
#include "DateTime.h"
#include "UnrealMemory.h"
#include "Containers/Array.h"

#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
#include "Engine/TextureDefines.h"
#include "Runtime/MovieScene/Public/MovieScene.h"
#else
#include "Engine/Classes/Engine/TextureDefines.h"
#endif

#include "PDRLuaObject.h"
#include "PDRLuaCppBinding.h"

#include "UObjectGlobals.h"
#include "GarbageCollection.h"


#if PLATFORM_ANDROID
#include "Runtime/Core/Public/Android/AndroidMisc.h"
#elif PLATFORM_IOS
#include "Runtime/Core/Public/IOS/IOSPlatformMisc.h"
#elif PLATFORM_WINDOWS
#include "Runtime/Core/Public/Windows/WindowsPlatformMisc.h"
#endif

namespace pandora
{
FString PCppInterface::GetPlatformDesc()
{
#if PLATFORM_WINDOWS
    return FString(TEXT("pc"));
#elif PLATFORM_MAC
    return FString(TEXT("mac"));
#elif PLATFORM_ANDROID
    return FString(TEXT("android"));
#elif PLATFORM_IOS
    return FString(TEXT("ios"));
#else
    return FString(TEXT("unknown"));
#endif
}

bool PCppInterface::IsCompatibleWithObsoletedLoadAssetMethod()
{
#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
    return true;
#else
    return false;
#endif
}

uint8 PCppInterface::GetNetworkType()
{
#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
    return 0;
#else
    return (uint8)FPlatformMisc::GetNetworkConnectionType();
#endif
}

int PCppInterface::CallGame(const FString & cmd)
{
    return PPandora::Get().CallGame(cmd);
}

UWorld * PCppInterface::GetWorld()
{
    return PPandora::Get().GetWorld();
}

UTexture2D* PCppInterface::LoadImageToTexture(const FString & path, int LODGroup, uint8 isSRGB)
{
    return PImageHelper::LoadImageToTexture2D(path, (TextureGroup)LODGroup, isSRGB);
}

void PCppInterface::UpdateTextureResources(UTexture2D* texture)
{
    PImageHelper::UpdateTextureResources(texture);
}

UWidgetAnimation* PCppInterface::GetWidgetAnimationByName(UUserWidget* widget, const FString & name)
{
     if (widget == nullptr)
     {
         PLog::LogError(TEXT("the target widget is null!"));
         return nullptr;
     }

    UProperty * prop = widget->GetClass()->PropertyLink;
    while (prop != nullptr)
    {
        if (prop->GetClass() == UObjectProperty::StaticClass())
        {
            UObjectProperty * objectProp = Cast<UObjectProperty>(prop);
            if (objectProp->PropertyClass == UWidgetAnimation::StaticClass())
            {
                UObject * obj = objectProp->GetObjectPropertyValue_InContainer(widget);
                UWidgetAnimation* anim = Cast<UWidgetAnimation>(obj);
                UMovieScene *movie = anim->GetMovieScene();
                if (anim && movie && movie->GetFName().ToString() == name)
                {
                    return anim;
                }
            }
        }
        prop = prop->PropertyLinkNext;
    }
    return nullptr;
}

void PCppInterface::ForceGarbageCollection()
{
    if (GEngine)
    {
        GEngine->ForceGarbageCollection(true);
    }
}

FString PCppInterface::GetHardwareInfo()
{
    TMap<FString, FString> info;
    info.Add(TEXT("memsize"), FString::FromInt(FGenericPlatformMemory::GetPhysicalGBRam()));
    if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
    {
        FVector2D viewportSize = GEngine->GameViewport->Viewport->GetSizeXY();
        info.Add(TEXT("viewportx"), FString::FromInt(viewportSize.X));
        info.Add(TEXT("viewporty"), FString::FromInt(viewportSize.Y));
    }

    info.Add(TEXT("resolutionx"), FString::FromInt(GSystemResolution.ResX));
    info.Add(TEXT("resolutiony"), FString::FromInt(GSystemResolution.ResY));

#if PLATFORM_ANDROID
    info.Add(TEXT("osversion"), FAndroidMisc::GetOSVersion());
    info.Add(TEXT("device"), FAndroidMisc::GetDeviceModel());
#elif PLATFORM_IOS
    info.Add(TEXT("osversion"), FIOSPlatformMisc::GetOSVersion());
    info.Add(TEXT("device"), FIOSPlatformMisc::GetDefaultDeviceProfileName());
#elif PLATFORM_WINDOWS
    info.Add(TEXT("osversion"), FWindowsPlatformMisc::GetOSVersion());
    info.Add(TEXT("device"), FWindowsPlatformMisc::GetDefaultDeviceProfileName());
#endif

    FString infostr = PCppUtilities::StringMapToJsonString(info);
    return infostr;
}

void PCppInterface::PlaySound(const FString& id)
{
    PPandora::Get().PlaySound(id);
}

FString PCppInterface::GetCurrency()
{
    TMap<FString, FString> results;
    PPandora::Get().GetCurrency(results);
    return PCppUtilities::StringMapToJsonString(results);
}

void PCppInterface::RefreshUserDataTokens()
{
    PPandora::Get().RefreshUserDataTokens();
}

void PCppInterface::Jump(const FString& type, const FString& content)
{
    PPandora::Get().Jump(type, content);
}

void PCppInterface::ShowItemTips(UCanvasPanel* anchor, const FString& itemID)
{
    PPandora::Get().ShowItemTips(anchor, itemID);
}

void PCppInterface::ShowItemIcon(UImage * image, const FString& itemID)
{
    PPandora::Get().ShowItemIcon(image, itemID);
}

void PCppInterface::ShowItem(UCanvasPanel* anchor, const FString& itemID, int itemCnt)
{
    PPandora::Get().ShowItem(anchor, itemID, itemCnt);
}

void PCppInterface::AddUserWidgetToGame(UUserWidget * userWidget, const FString & panelName)
{
    PPandora::Get().AddUserWidgetToGame(userWidget, panelName);
}

void PCppInterface::RemoveUserWidgetFromGame(UUserWidget * userWidget, const FString& panelName)
{
    PPandora::Get().RemoveUserWidgetFromGame(userWidget, panelName);
}

//void PCppInterface::AddUObjectToRoot(UObject* obj)
//{
//    if (obj == nullptr)
//        return;
//
//    if (obj->IsPendingKill())
//        return;
//
//    if (obj->IsRooted())
//        return;
//
//
//    obj->AddToRoot();
//}
//
//void PCppInterface::RemoveUObjectFromRoot(UObject* obj) 
//{
//    if (obj == nullptr) 
//        return;
//
//    if (obj->IsPendingKill())
//        return;
//
//    if (!obj->IsRooted())
//        return;
//
//
//    obj->RemoveFromRoot();
//}

int64 PCppInterface::GetNowInMs()
{
    FDateTime now = FDateTime::Now();
    int64 inMs = now.ToUnixTimestamp() * 1000 + now.GetMillisecond();
    return inMs;
}
}

namespace NS_PDR_SLUA
{
using pandora::PCppInterface;
DefLuaClass(PCppInterface)
    DefLuaMethod(GetPlatformDesc, &PCppInterface::GetPlatformDesc)
    DefLuaMethod(IsCompatibleWithObsoletedLoadAssetMethod, &PCppInterface::IsCompatibleWithObsoletedLoadAssetMethod)
    DefLuaMethod(GetNetworkType, &PCppInterface::GetNetworkType)
    DefLuaMethod(CallGame, &PCppInterface::CallGame)
    DefLuaMethod(GetWorld, &PCppInterface::GetWorld)
    DefLuaMethod(LoadImageToTexture, &PCppInterface::LoadImageToTexture)
    DefLuaMethod(UpdateTextureResources, &PCppInterface::UpdateTextureResources)
    DefLuaMethod(GetWidgetAnimationByName, &PCppInterface::GetWidgetAnimationByName)
    DefLuaMethod(ForceGarbageCollection, &PCppInterface::ForceGarbageCollection)
    DefLuaMethod(GetHardwareInfo, &PCppInterface::GetHardwareInfo)
    DefLuaMethod(PlaySound, &PCppInterface::PlaySound)
    DefLuaMethod(GetCurrency, &PCppInterface::GetCurrency)
    DefLuaMethod(RefreshUserDataTokens, &PCppInterface::RefreshUserDataTokens)
    DefLuaMethod(Jump, &PCppInterface::Jump)
    DefLuaMethod(ShowItemTips, &PCppInterface::ShowItemTips)
    DefLuaMethod(ShowItemIcon, &PCppInterface::ShowItemIcon)
    DefLuaMethod(ShowItem, &PCppInterface::ShowItem)
    DefLuaMethod(AddUserWidgetToGame, &PCppInterface::AddUserWidgetToGame)
    DefLuaMethod(RemoveUserWidgetFromGame, &PCppInterface::RemoveUserWidgetFromGame)
    //DefLuaMethod(AddUObjectToRoot, &PCppInterface::AddUObjectToRoot)
    //DefLuaMethod(RemoveUObjectFromRoot, &PCppInterface::RemoveUObjectFromRoot)
    DefLuaMethod(GetNowInMs, &PCppInterface::GetNowInMs)
EndDef(PCppInterface, nullptr)
}
