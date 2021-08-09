#pragma once

#include "Containers/Map.h"
#include "UnrealString.h"

namespace pandora
{
class PCppUtilities
{
public:
    static FString StringMapToJsonString(const TMap<FString, FString>& input);
};
}

