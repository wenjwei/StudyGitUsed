#pragma once

#include "CoreMinimal.h"
#include "UserWidget.h"
#include "BulletScreenStructs.h"

#include "BulletScreenItem.generated.h"


UCLASS()
class PANDORA_API UBulletScreenItem : public UUserWidget
{
    GENERATED_UCLASS_BODY()
private:
    FVector2D cachedSize;

public:
    void SetItemInfo(pandora::components::bulletscreen::PtrBulletItemInfo info);
    FVector2D GetItemSize();
};
