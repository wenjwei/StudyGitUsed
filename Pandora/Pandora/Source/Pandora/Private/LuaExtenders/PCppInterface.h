#pragma once
// utilities implemented by cpp for lua

#include "SharedPointer.h"
#include "Containers/UnrealString.h"
#include "Math/Vector2D.h"

class UWorld;
class UTexture2D;
class UWidgetAnimation;
class UUserWidget;
class UCanvasPanel;
class UImage;

namespace pandora
{
class PCppInterface
{
public:
    static FString GetPlatformDesc();
    static bool IsCompatibleWithObsoletedLoadAssetMethod();
    static uint8 GetNetworkType();
    static int CallGame(const FString & cmd);
    static UWorld * GetWorld();
    static UTexture2D* LoadImageToTexture(const FString & path, int LODGroup, uint8 isSRGB);
    static void UpdateTextureResources(UTexture2D* texture);
    static UWidgetAnimation* GetWidgetAnimationByName(UUserWidget*, const FString & name);
    static void ForceGarbageCollection();
    static FString GetHardwareInfo();
    static void PlaySound(const FString& id);
    static FString GetCurrency();
    static void RefreshUserDataTokens();
    static void Jump(const FString& type, const FString& content);
    static void ShowItemTips(UCanvasPanel* anchor, const FString& itemID);
    static void ShowItemIcon(UImage * image, const FString& itemID);
    static void ShowItem(UCanvasPanel* anchor, const FString& itemID, int itemCnt);
    static void AddUserWidgetToGame(UUserWidget *, const FString& panelName);
    static void RemoveUserWidgetFromGame(UUserWidget *, const FString& panelName);
    //static void AddUObjectToRoot(UObject* obj);
    //static void RemoveUObjectFromRoot(UObject* obj);
    static int64 GetNowInMs();
};
}
