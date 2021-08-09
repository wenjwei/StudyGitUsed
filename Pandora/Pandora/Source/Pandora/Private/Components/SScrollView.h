#pragma once

#include "CoreMinimal.h"
#include "SScrollBox.h"

class SScrollView : public SScrollBox
{
public:

	DECLARE_DELEGATE(OnTouchDelegate);

	//UPROPERTY(BlueprintAssignable, Category="ScrollView")
	OnTouchDelegate onTouchStartDelegate;
	OnTouchDelegate onTouchEndDelegate;

	virtual FReply OnTouchStarted(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent);
	virtual FReply OnTouchEnded(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent) override;
};