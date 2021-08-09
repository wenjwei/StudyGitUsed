#pragma once

#include "CoreMinimal.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "Regex.h"
#include "Scale9Grid.generated.h"

UCLASS()
class UScale9Grid : public UImage
{
    GENERATED_UCLASS_BODY()
public:
    virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	//~ Begin UWidget Interface
	virtual const FText GetPaletteCategory() override;
	//~ End UWidget Interface
#endif

protected:
    TSharedPtr<SImage> MyImage;

    virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
    const FSlateBrush* ConvertImage(TAttribute<FSlateBrush> InImageAsset) const;

private:
    const FRegexPattern SCALE_9_EXP = FRegexPattern(TEXT("(?<=-)l\\d+_r\\d+_t\\d+_b\\d+"));
    const FRegexPattern SCALE_9_LEFT = FRegexPattern(TEXT("(?<=l)\\d+(?=_)"));
    const FRegexPattern SCALE_9_RIGHT = FRegexPattern(TEXT("(?<=r)\\d+(?=_)"));
    const FRegexPattern SCALE_9_TOP = FRegexPattern(TEXT("(?<=t)\\d+(?=_)"));
    const FRegexPattern SCALE_9_BOTTOM = FRegexPattern(TEXT("(?<=b)\\d+(?=_)"));
	const FRegexPattern SCALE_9_WIDTH = FRegexPattern(TEXT("(?<=w)\\d+(?=_)"));
	const FRegexPattern SCALE_9_HEIGHT = FRegexPattern(TEXT("(?<=h)\\d+"));

    bool IsScale9Format(FString name) const;
    float GetNumberFromRegex(FRegexPattern pattern, FString str) const;
    FMargin ParseGridData(UPaperSprite* sprite) const;
};
