
#include "BulletScreenItem.h"
#include "CoreMinimal.h"
#include "Components/TextBlock.h"
#include "SlateBlueprintLibrary.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "UMG"

UBulletScreenItem::UBulletScreenItem(const FObjectInitializer& ObjectInitializer) : 
    Super(ObjectInitializer),
    cachedSize(FVector2D(0, 0))
{

}

void UBulletScreenItem::SetItemInfo(pandora::components::bulletscreen::PtrBulletItemInfo info)
{
    UWidget * widget = GetWidgetFromName(FName(TEXT("ContentText")));
    if (!widget)
    {
        // warning
        return;
    }

    UTextBlock * text = Cast<UTextBlock>(widget);
    if (!text)
    {
        // warning
        return;
    }

    FText textContent = FText::FromString(info->content);
    text->SetText(textContent);
    TSharedRef<FSlateFontMeasure> fontMesureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    cachedSize = fontMesureService->Measure(textContent, text->Font, 1.0);
}

FVector2D UBulletScreenItem::GetItemSize()
{
    return cachedSize;
}

#undef LOCTEXT_NAMESPACE

