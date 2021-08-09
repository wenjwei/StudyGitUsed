#include "BulletScreen.h"
#include "Containers/Map.h"
#include "Classes/Engine/GameInstance.h"
#include "SlateBlueprintLibrary.h"
#include "Runtime/Launch/Resources/Version.h"
#include "TimerManager.h"
#if WITH_EDITOR
#include "Classes/Editor/UnrealEdEngine.h"
#else
#include "Classes/Engine/GameEngine.h"
#endif


#define LOCTEXT_NAMESPACE "UMG"

UBulletScreen::UBulletScreen(const FObjectInitializer& objectInitializer) :
    Super(objectInitializer),
    MaxBulletLines(4),
    BulletSpeed(100),
    LineHeight(1),
    HorizontalInterval(1),
    MaxCachedBulletCount(10),
    cachedCount(0),
    tickInterval(0.03),
    bIsStarted(false)
    //lastTickTime(0)
{
    Clipping = EWidgetClipping::ClipToBounds;
}

void UBulletScreen::Start()
{
    if (bIsStarted)
        return;
    UGameInstance* gameInstance = GetGameInstance();
    if (!gameInstance)
        return;

    if (LineHeight <= 0)
    {
        // pop warning
        return;
    }

    halfLineHeight = LineHeight / 2;
    //lastTickTime = GetNowInMs();
    FTimerDelegate timerDelegate;
    timerDelegate.BindUObject(this, &UBulletScreen::Tick);
    gameInstance->GetTimerManager().SetTimer(timerHandle, timerDelegate, tickInterval, true);
    bIsStarted = true;
}

void UBulletScreen::Pause()
{
    if (!bIsStarted)
        return;

    UGameInstance* gameInstance = GetGameInstance();
    if (!gameInstance)
        return;

    gameInstance->GetTimerManager().PauseTimer(timerHandle);
}

void UBulletScreen::Resume()
{
    if (!bIsStarted)
        return;

    UGameInstance* gameInstance = GetGameInstance();
    if (!gameInstance)
        return;

    //lastTickTime = GetNowInMs();
    gameInstance->GetTimerManager().UnPauseTimer(timerHandle);
}

void UBulletScreen::Stop()
{
    if (!bIsStarted)
        return;

    UGameInstance* gameInstance = GetGameInstance();
    if (!gameInstance)
        return;
    gameInstance->GetTimerManager().ClearTimer(timerHandle);
    // clear cachedInfo and off-screen all on-screen items
    cachedInfo.Empty();
    cachedCount = 0;
    TDoubleLinkedList<PtrOnScreenBulletItem>::TIterator iter(onScreenItems.GetHead());
    for (; (bool)iter; ++iter) 
    {
        MakeItemOffScreen(*iter);
    }
    onScreenItems.Empty();
    bIsStarted = false;
}

void UBulletScreen::AddBullet(const FString& content)
{
    pandora::components::bulletscreen::PtrBulletItemInfo item;
    if (bulletInfoObjPool.Num() > 0)
        item = bulletInfoObjPool.Pop();
    else
        item = MakeShareable(new pandora::components::bulletscreen::BulletItemInfo);

    item->content = content;
    item->style = pandora::components::bulletscreen::PtrBulletItemStyle(nullptr);
    PushOnScreenItem(item);
}

void UBulletScreen::AddBullets(const TArray<FString>& content)
{
    for (int i = 0; i < content.Num(); ++i)
    {
        AddBullet(content[i]);
    }
}

void UBulletScreen::BeginDestroy()
{
    // if is started, stop timer
    if (bIsStarted)
    {
        UGameInstance* gameInstance = GetGameInstance();
        if (gameInstance)
        {
            gameInstance->GetTimerManager().ClearTimer(timerHandle);
        }
        bIsStarted = false;
    }

    // clear all containers before destroy
    cachedInfo.Empty();
    cachedCount = 0;
    bulletInfoObjPool.Empty();
    bulletItemPool.Empty();
    onScreenItems.Empty();
    Super::BeginDestroy();
}

void UBulletScreen::Tick()
{
    // in millisecond
    //int64 now = GetNowInMs();
    //int64 deltaTime = now - lastTickTime;
    //lastTickTime = now;

    // iterate over on-screen bullets
    // update activated bullets
    // remove dead bullets
    TDoubleLinkedList<PtrOnScreenBulletItem>::TIterator iter(onScreenItems.GetHead());
    offScreenItems.Empty();
    lineTailPositionMark.Empty();
    for (; (bool)iter; ++iter)
    {
        if (ShouldItemOffScreen(*iter))
        {
            offScreenItems.Add(iter.GetNode());
            MakeItemOffScreen(*iter);
        }
        else
        {
            UpdateOnScreenItem(*iter);
        }
    }
    for (int i = 0; i < offScreenItems.Num(); ++i)
    {
        onScreenItems.RemoveNode(offScreenItems[i]);
    }
    offScreenItems.Empty();

    // on screen multi bullets per tick
    int lineIndex = GetAvailableLine();
    while (lineIndex >= 0 && !cachedInfo.IsEmpty())
    {
        pandora::components::bulletscreen::PtrBulletItemInfo info = PopOnScreenItem();
        if (info.IsValid())
        {
            // item on screen
            MakeItemOnScreen(info, lineIndex);
        }
        lineIndex = GetAvailableLine();
    }
    lineTailPositionMark.Empty();
}

UGameInstance* UBulletScreen::GetGameInstance()
{
    UGameInstance* GameInstance = nullptr;
#if WITH_EDITOR
    UUnrealEdEngine* engine = Cast<UUnrealEdEngine>(GEngine);
    if (engine && engine->PlayWorld) GameInstance = engine->PlayWorld->GetGameInstance();
#else
    UGameEngine* engine = Cast<UGameEngine>(GEngine);
    if (engine) GameInstance = engine->GameInstance;
#endif
 
    return GameInstance;
}

int UBulletScreen::PushOnScreenItem(pandora::components::bulletscreen::PtrBulletItemInfo item)
{
    if (cachedCount >= MaxCachedBulletCount)
        return 1;
    if (!cachedInfo.Enqueue(item))
        return 2;
    cachedCount += 1;
    return 0;
}

pandora::components::bulletscreen::PtrBulletItemInfo UBulletScreen::PopOnScreenItem()
{
    if (cachedInfo.IsEmpty())
        return pandora::components::bulletscreen::PtrBulletItemInfo(nullptr);
    pandora::components::bulletscreen::PtrBulletItemInfo result;
    if (!cachedInfo.Dequeue(result))
        return pandora::components::bulletscreen::PtrBulletItemInfo(nullptr);
    cachedCount -= 1;
    return result;
}

//int64 UBulletScreen::GetNowInMs()
//{
//    FDateTime now = FDateTime::UtcNow();
//    int64 timestampNow = now.ToUnixTimestamp();
//    return timestampNow * 1000 + now.GetMillisecond();
//}

UCanvasPanelSlot* UBulletScreen::GetBulletWidgetSlot()
{
    if (bulletItemPool.Num() > 0)
    {
        return bulletItemPool.Pop();
    }

    if (TemplateWidget.Get() == nullptr)
    {
        return nullptr;
    }


#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
    UGameInstance* gameInstance = GetGameInstance();
    if (!gameInstance) return nullptr;
    UWidget* bullet = CreateWidget<UWidget>(gameInstance, TemplateWidget.Get());
#else
    UBulletScreenItem* bullet = Cast<UBulletScreenItem>(CreateWidget(this, TemplateWidget.Get()));
#endif
    return AddChildToCanvas(bullet);
}

float UBulletScreen::GetScreenSizeX()
{
    FVector2D size = USlateBlueprintLibrary::GetLocalSize(GetCachedGeometry());
    //UE_LOG(LogTemp, Warning, TEXT("BulletScreenSize: x-%d, y-%d"), (int)size.X, (int)size.Y);
    return size.X;
}

void UBulletScreen::MakeItemOnScreen(pandora::components::bulletscreen::PtrBulletItemInfo info, int lineIndex)
{
    PtrOnScreenBulletItem item = MakeShareable(new OnScreenBulletItem);
    item->widgetSlot = GetBulletWidgetSlot();
    item->info = info;
    float posX = GetScreenSizeX() + 10;
    float posY = lineIndex * LineHeight;
    item->widgetSlot->SetPosition(FVector2D(posX, posY));
    item->widgetSlot->Content->SetVisibility(ESlateVisibility::HitTestInvisible);
    UBulletScreenItem* bullet = Cast<UBulletScreenItem>(item->widgetSlot->Content);
    if (!bullet)
    {
        bulletInfoObjPool.Push(item->info);
        item->widgetSlot->Content->RemoveFromParent();
        return;
    }
    if (lineTailPositionMark.Contains(lineIndex))
        lineTailPositionMark[lineIndex] = posX + bullet->GetItemSize().X;
    else
        lineTailPositionMark.Add(lineIndex, posX + bullet->GetItemSize().X);

    bullet->SetItemInfo(info);
    item->widgetSlot->SetSize(bullet->GetItemSize());
    onScreenItems.AddTail(item);
}

void UBulletScreen::MakeItemOffScreen(PtrOnScreenBulletItem item)
{
    item->widgetSlot->Content->SetVisibility(ESlateVisibility::Hidden);
    bulletInfoObjPool.Push(item->info);
    bulletItemPool.Push(item->widgetSlot);
    item->widgetSlot = nullptr;
}

void UBulletScreen::UpdateOnScreenItem(PtrOnScreenBulletItem item)
{
    FVector2D oldPosition = item->widgetSlot->GetPosition();
    float x = oldPosition.X - tickInterval * BulletSpeed;
    FVector2D newPosition = FVector2D(x, oldPosition.Y);
    item->widgetSlot->SetPosition(newPosition);
    int line = FMath::FloorToInt((oldPosition.Y + halfLineHeight) / LineHeight);

    // update tail mark
    // notice that the lineTailPositionMark will be empty in the very beginning of every tick
    // to make sure we get the correct data
    UBulletScreenItem* bullet = Cast<UBulletScreenItem>(item->widgetSlot->Content);
    float itemTail = x + bullet->GetItemSize().X;
    float tailPositionMark = 0.0;
    if (lineTailPositionMark.Contains(line))
        tailPositionMark = lineTailPositionMark[line];
    if (itemTail > tailPositionMark)
    {
        if (lineTailPositionMark.Contains(line))
            lineTailPositionMark[line] = itemTail;
        else
            lineTailPositionMark.Add(line, itemTail);
    }
}

bool UBulletScreen::ShouldItemOffScreen(PtrOnScreenBulletItem item)
{
    FVector2D position = item->widgetSlot->GetPosition();
    UBulletScreenItem* bullet = Cast<UBulletScreenItem>(item->widgetSlot->Content);
    if (!bullet)
        return true;

    if (position.X + bullet->GetItemSize().X < 0)
        return true;

    return false;
}

int UBulletScreen::GetAvailableLine()
{
    float sizeX = GetScreenSizeX();
    for (int i = 0; i < MaxBulletLines; ++i)
    {
        if (!lineTailPositionMark.Contains(i))
            return i;
        if (lineTailPositionMark[i] + HorizontalInterval < sizeX)
            return i;
    }
    return -1;
}

#undef LOCTEXT_NAMESPACE

