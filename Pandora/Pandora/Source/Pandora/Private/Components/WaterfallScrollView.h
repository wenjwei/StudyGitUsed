#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Components/PanelWidget.h"
#include "SScrollView.h"
#include "DelegateCombinations.h"
#include "PDRLuaObject.h"
#include "PDRLuaVar.h"
#include "PDRLuaDelegate.h"
#include "WaterfallScrollView.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRefreshDelegate);

UCLASS()
class PANDORA_API UWaterfallScrollView : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pandora", meta = (DisplayName = "Style"))
		FScrollBoxStyle WidgetStyle;

	UPROPERTY()
		USlateWidgetStyleAsset* Style_DEPRECATED;

	UPROPERTY(EditAnywhere, Category = "Pandora")
		TSubclassOf<UUserWidget> TemplateWidget;

	UPROPERTY(BlueprintReadWrite, Category = "Pandora")
		int ItemCount;

	UPROPERTY(BlueprintReadWrite, Category = "Pandora")
		int ColumnCount;

	UFUNCTION(BlueprintCallable, Category = "Pandora")
		void Fill();

	UFUNCTION(BlueprintCallable, Category = "Pandora")
		void Clear();

	UFUNCTION(BlueprintCallable, Category = "Pandora")
		void ScrollToStart();

	UFUNCTION(BlueprintCallable, Category = "Pandora")
		void RefreshLayout(int index);

	UPROPERTY(BlueprintAssignable, Category = "Pandora")
		FOnRefreshDelegate onReachTop;

	UPROPERTY(BlueprintAssignable, Category = "Pandora")
		FOnRefreshDelegate onReachBottom;

	/** When mouse wheel events should be consumed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scroll")
		EConsumeMouseWheel ConsumeMouseWheel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scroll")
		bool AllowOverscroll;

	NS_PDR_SLUA::LuaVar onFillItem;
	NS_PDR_SLUA::LuaVar onRecycleItem;

	//~ Begin UWidget Interface
	virtual void SynchronizeProperties() override;
	//~ End UWidget Interface

	//~ Begin UVisual Interface
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~ End UVisual Interface

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	//~ End UObject Interface

#if WITH_EDITOR
	//~ Begin UWidget Interface
	virtual const FText GetPaletteCategory() override;
	virtual void OnDescendantSelectedByDesigner(UWidget* DescendantWidget) override;
	virtual void OnDescendantDeselectedByDesigner(UWidget* DescendantWidget) override;
	//~ End UWidget Interface
#endif

protected:
	// UPanelWidget
	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

	float desiredScrollOffset;
	TSharedPtr<class SScrollView> ScrollView;

	//~ Begin UWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	//~ End UWidget Interface

private:

	float REFRESH_SENSITIVITY = 30;
	float LOADMORE_OFFSET_RANGE = 2.5;

	float topBoundary;
	float bottomBoundary;
	float offsetY;
	float touchStartOffset;
	float touchEndOffset;
	float lastOffset;
	bool canRefresh = true;

	TArray<TArray<UWidget*>> visibleItemList;
	TArray<UWidget*> recycleItemList;
	TMap<int, TArray<int>> logicIndexMap;
	TMap<int, TArray<int>> recycleIndexMap;

	void CreateSubContent();
	UWidget* GetContainer();
	void ScrollWidgetIntoView(UWidget* WidgetToFind, bool AnimateScroll = true, EDescendantScrollDestination ScrollDestination = EDescendantScrollDestination::IntoView);

	void OnTouchStart();
	void OnTouchEnd();

	void Initiate();
	void Forward();
	void Backward();
	float GetItemPosition(UWidget* item);
	float GetItemHeight(UWidget* item);
	float GetItemWidth(UWidget* item);
	UWidget* GetTopItem(int column);
	UWidget* GetBottomItem(int column);
	int TopColumnToAddItem();
	int TopColumnToRecycleItem();
	void AddTopItem(int column);
	int BottomColumnToAddItem();
	int BottomColumnToRecycleItem();
	void AddBottomItem(int column);
	void RecycleTopItem(int column);
	void RecycleBottomItem(int column);
	UWidget* GetItem();
	void OnScrolledValueChange(float currentOffset);

	int GetTotalNum(); //get total num of item already add to view;
	int GetVisibleItemNum();
	int GetLastIndex();
	int GetFirstIndex();
	int GetMinLogicIndex();
	float GetLastBottomPosition(); // get the lowest Item bottom Position
	bool IsScrolling(float curOffset);

#if WITH_EDITOR
	FDelegateHandle TickHandle;
#endif //WITH_EDITOR

};