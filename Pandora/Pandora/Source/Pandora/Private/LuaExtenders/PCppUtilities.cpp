
#include "PCppUtilities.h"
#include "Runtime/Json/Public/Json.h"

namespace pandora
{
FString PCppUtilities::StringMapToJsonString(const TMap<FString, FString>& input)
{
    TSharedPtr<FJsonObject> obj = MakeShareable(new FJsonObject);
    for (const TPair<FString, FString>& item : input)
    {
        obj->SetStringField(item.Key, item.Value);
    }
    FString output;
    TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&output);
    FJsonSerializer::Serialize(obj.ToSharedRef(), writer);
    return output;
}
}

