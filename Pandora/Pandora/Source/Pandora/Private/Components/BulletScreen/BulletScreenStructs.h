#pragma once

#include "SharedPointer.h"

namespace pandora 
{
namespace components
{
namespace bulletscreen
{
typedef struct t_OnScreenItemStyle {
} BulletItemStyle;
typedef TSharedPtr<BulletItemStyle> PtrBulletItemStyle;

typedef struct t_OnScreenItem {
    FString content;
    PtrBulletItemStyle style;
} BulletItemInfo;
typedef TSharedPtr<BulletItemInfo> PtrBulletItemInfo;

}
}
}

