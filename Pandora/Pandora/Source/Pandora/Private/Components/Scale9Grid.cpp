#include "Scale9Grid.h"
#include "Slate.h"

#define LOCTEXT_NAMESPACE "UMG"

UScale9Grid::UScale9Grid(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
{

}

#if WITH_EDITOR
const FText UScale9Grid::GetPaletteCategory()
{
	return LOCTEXT("Pandora", "Pandora Widget");
}
#endif

const FSlateBrush* UScale9Grid::ConvertImage(TAttribute<FSlateBrush> InImageAsset) const
{
    auto* MutableThis = const_cast<UScale9Grid*>( this );
    auto* resource = InImageAsset.Get().GetResourceObject();
    FSlateBrush brush = InImageAsset.Get();
    if (resource == nullptr)
    {
        MutableThis->Brush = brush;
        return &Brush;
    }
    auto* sprite = Cast<UPaperSprite>(resource);
    if (IsScale9Format(resource->GetName()) && sprite)
    {
        FMargin margin = ParseGridData(sprite);
        brush.DrawAs = ESlateBrushDrawType::Box;
        brush.Margin = margin;
    }
    MutableThis->Brush = brush;
    return &Brush;
}

TSharedRef<SWidget> UScale9Grid::RebuildWidget()
{
    MyImage = SNew(SImage);
    return MyImage.ToSharedRef();
}

void UScale9Grid::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyImage.Reset();
}

void UScale9Grid::SynchronizeProperties()
{

    Super::SynchronizeProperties();

    TAttribute<FSlateColor> ColorAndOpacityBinding = PROPERTY_BINDING(FSlateColor, ColorAndOpacity);
    TAttribute<const FSlateBrush*> ImageBinding = OPTIONAL_BINDING_CONVERT(FSlateBrush, Brush, const FSlateBrush*, ConvertImage);

    if (MyImage.IsValid())
    {
        MyImage->SetImage(ImageBinding);
        MyImage->SetColorAndOpacity(ColorAndOpacityBinding);
        MyImage->SetOnMouseButtonDown(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleMouseButtonDown));
    }
}

bool UScale9Grid::IsScale9Format(FString name) const
{
    FRegexMatcher matcher(SCALE_9_EXP, name);
    return matcher.FindNext();
}

float UScale9Grid::GetNumberFromRegex(FRegexPattern pattern, FString str) const
{
    FRegexMatcher matcher(pattern, str);
    if (!matcher.FindNext())
    {
        return 0;
    }
    FString result = matcher.GetCaptureGroup(0);
    return FCString::Atof(*result);
}

FMargin UScale9Grid::ParseGridData(UPaperSprite* sprite) const
{
    FString name = sprite->GetName();
    float left = GetNumberFromRegex(SCALE_9_LEFT, name);
    float right = GetNumberFromRegex(SCALE_9_RIGHT, name);
    float top = GetNumberFromRegex(SCALE_9_TOP, name);
    float bottom = GetNumberFromRegex(SCALE_9_BOTTOM, name);
	float width = GetNumberFromRegex(SCALE_9_WIDTH, name);
	float height = GetNumberFromRegex(SCALE_9_HEIGHT, name);
    return FMargin((left / width), top / height, right / width, bottom / height);
}

#undef LOCTEXT_NAMESPACE
