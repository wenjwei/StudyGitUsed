#pragma once

#include "CoreMinimal.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Containers/Queue.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "BulletScreenStructs.h"
#include "BulletScreenItem.h"
#include "BulletScreen.generated.h"

class UGameIntance;

UCLASS()
class PANDORA_API UBulletScreen : public UCanvasPanel
{
    GENERATED_UCLASS_BODY() 

public:
    typedef struct t_OnScreenBulletItem {
        UCanvasPanelSlot* widgetSlot;
        pandora::components::bulletscreen::PtrBulletItemInfo info;
    } OnScreenBulletItem;
    typedef TSharedPtr<OnScreenBulletItem> PtrOnScreenBulletItem;

public:

    UPROPERTY(EditAnywhere, Category="BulletScreen")
    TSubclassOf<UBulletScreenItem> TemplateWidget;

    UFUNCTION(BlueprintCallable, Category="BulletScreen")
    void Start();

    UFUNCTION(BlueprintCallable, Category="BulletScreen")
    void Pause();

    UFUNCTION(BlueprintCallable, Category="BulletScreen")
    void Resume();

    UFUNCTION(BlueprintCallable, Category="BulletScreen")
    void Stop();

    UFUNCTION(BlueprintCallable, Category="BulletScreen")
    void AddBullet(const FString& content);

    UFUNCTION(BlueprintCallable, Category="BulletScreen")
    void AddBullets(const TArray<FString>& content);

    //UFUNCTION(BlueprintCallable, Category="BulletScreen")
    //void AddColoredBullet(const FString& content, const FColor& color);

    UPROPERTY(EditAnywhere, Category="BulletScreen")
    int MaxBulletLines;

    UPROPERTY(EditAnywhere, Category="BulletScreen")
    float BulletSpeed;

    UPROPERTY(EditAnywhere, Category="BulletScreen")
    float LineHeight;

    UPROPERTY(EditAnywhere, Category="BulletScreen")
    float HorizontalInterval;

    UPROPERTY(EditAnywhere, Category="BulletScreen")
    int MaxCachedBulletCount;

    virtual void BeginDestroy() override;

    void Tick();

protected:
    //virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);

private:
    //TArray<pandora::PtrOnScreenItem> onScreenItemPool;
    int cachedCount;
    float halfLineHeight;  // for line index calculation
    float tickInterval;
    //int64 lastTickTime; // unix timestamp in ms
    TQueue<pandora::components::bulletscreen::PtrBulletItemInfo> cachedInfo;
    TArray<pandora::components::bulletscreen::PtrBulletItemInfo> bulletInfoObjPool;
    TArray<UCanvasPanelSlot*> bulletItemPool;
    TMap<int, float> lineTailPositionMark;

    // update this list in tick
    TDoubleLinkedList<PtrOnScreenBulletItem> onScreenItems;
    TArray<TDoubleLinkedList<PtrOnScreenBulletItem>::TDoubleLinkedListNode*> offScreenItems;

    FTimerHandle timerHandle;
    bool bIsStarted;

    UGameInstance* GetGameInstance();

    int PushOnScreenItem(pandora::components::bulletscreen::PtrBulletItemInfo item);
    pandora::components::bulletscreen::PtrBulletItemInfo PopOnScreenItem();
    //int64 GetNowInMs();
    UCanvasPanelSlot* GetBulletWidgetSlot();
    PtrOnScreenBulletItem GetOnScreenItem();

    float GetScreenSizeX();
    void MakeItemOnScreen(pandora::components::bulletscreen::PtrBulletItemInfo info, int lineIndex);
    void MakeItemOffScreen(PtrOnScreenBulletItem item);
    void UpdateOnScreenItem(PtrOnScreenBulletItem item);
    bool ShouldItemOffScreen(PtrOnScreenBulletItem item);
    int GetAvailableLine();
};

