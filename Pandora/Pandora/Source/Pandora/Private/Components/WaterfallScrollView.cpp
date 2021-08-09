#include "WaterfallScrollView.h"
#include "Containers/Ticker.h"
#include "WidgetTree.h"
#include "CanvasPanel.h"
#include "CanvasPanelSlot.h"
#include "SlateBlueprintLibrary.h"
#include "Components/ScrollBoxSlot.h"

#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#include "Editor/UnrealEdEngine.h"
#else
#include "Engine/GameEngine.h"
#endif

#define LOCTEXT_NAMESPACE "UMG"

UWaterfallScrollView::UWaterfallScrollView(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
	, ConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible)
	, AllowOverscroll(true)
{
	bIsVariable = false;
	SScrollView::FArguments defaltArgs;
	Visibility = UWidget::ConvertRuntimeToSerializedVisibility(defaltArgs._Visibility.Get());
	Clipping = EWidgetClipping::ClipToBounds;
	WidgetStyle = *defaltArgs._Style;
	Initiate();
}

void UWaterfallScrollView::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ScrollView->SetScrollOffset(desiredScrollOffset);
	ScrollView->SetScrollBarVisibility(UWidget::ConvertSerializedVisibilityToRuntime(ESlateVisibility::Collapsed));
	ScrollView->SetAllowOverscroll(AllowOverscroll ? EAllowOverscroll::Yes : EAllowOverscroll::No);
	ScrollView->SetScrollBarRightClickDragAllowed(true);
}

void UWaterfallScrollView::ReleaseSlateResources(bool releaseChildren)
{
	Super::ReleaseSlateResources(releaseChildren);
	ScrollView.Reset();
}

void UWaterfallScrollView::PostLoad()
{
	Super::PostLoad();
}

//////////////////  With Editor 
#if WITH_EDITOR

const FText UWaterfallScrollView::GetPaletteCategory()
{
	return LOCTEXT("Pandora", "Pandora Widget");
}

void UWaterfallScrollView::OnDescendantSelectedByDesigner(UWidget* descendantWidget)
{
	UWidget* selectedChild = UWidget::FindChildContainingDescendant(this, descendantWidget);
	if (selectedChild)
	{
		ScrollWidgetIntoView(selectedChild, true);
		if (TickHandle.IsValid())
		{
			FTicker::GetCoreTicker().RemoveTicker(TickHandle);
			TickHandle.Reset();
		}
	}
}

void UWaterfallScrollView::OnDescendantDeselectedByDesigner(UWidget* descendantWidget)
{
	if (TickHandle.IsValid())
	{
		FTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
	TickHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=](float) -> bool
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_UScrollBox_ScrollToStart_LambdaTick);
			this->ScrollToStart();
			return false;
		}));
}

#endif

UClass* UWaterfallScrollView::GetSlotClass() const
{
	return UScrollBoxSlot::StaticClass();
}

void UWaterfallScrollView::OnSlotAdded(UPanelSlot* slot)
{
	if (ScrollView.IsValid())
	{
		CastChecked<UScrollBoxSlot>(slot)->BuildSlot(ScrollView.ToSharedRef());
	}
}

void UWaterfallScrollView::OnSlotRemoved(UPanelSlot* slot)
{
	if (ScrollView.IsValid())
	{
		TSharedPtr<SWidget> widget = slot->Content->GetCachedWidget();
		if (widget.IsValid())
		{
			ScrollView->RemoveSlot(widget.ToSharedRef());
		}
	}
}

TSharedRef<SWidget> UWaterfallScrollView::RebuildWidget()
{
	ScrollView = SNew(SScrollView)
		.Style(&WidgetStyle)
		.Orientation(EOrientation::Orient_Vertical)
		.ConsumeMouseWheel(ConsumeMouseWheel)
		.OnUserScrolled_Lambda([this](float currentOffset)
			{
				OnScrolledValueChange(currentOffset);
			});
	ScrollView->onTouchStartDelegate.BindUObject(this, &UWaterfallScrollView::OnTouchStart);
	ScrollView->onTouchEndDelegate.BindUObject(this, &UWaterfallScrollView::OnTouchEnd);
	for (UPanelSlot* panelSlot : Slots)
	{
		if (UScrollBoxSlot* scrollBoxSlot = Cast<UScrollBoxSlot>(panelSlot))
		{
			scrollBoxSlot->Parent = this;
			scrollBoxSlot->BuildSlot(ScrollView.ToSharedRef());
		}
	}
#if WITH_EDITOR
	if (GetChildrenCount() == 0 && CanAddMoreChildren())
	{
		CreateSubContent();
	}
	bIsVariable = true;
#endif // WITH_EDITOR 
	return ScrollView.ToSharedRef();
}

void UWaterfallScrollView::ScrollWidgetIntoView(UWidget* widgetToFind, bool animateScroll, EDescendantScrollDestination scrollDestination)
{
	TSharedPtr<SWidget> slateWidgetToFind;
	if (widgetToFind)
	{
		slateWidgetToFind = widgetToFind->GetCachedWidget();
	}
	if (ScrollView.IsValid())
	{
		ScrollView->ScrollDescendantIntoView(slateWidgetToFind, animateScroll, scrollDestination);
	}
}

void UWaterfallScrollView::ScrollToStart()
{
	if (ScrollView.IsValid())
	{
		ScrollView->ScrollToStart();
		UCanvasPanel* container = CastChecked<UCanvasPanel>(GetContainer());
		container->ClearChildren();
		Initiate();
		Fill();
	}
}

/*****************************                         ************************************/

void UWaterfallScrollView::OnScrolledValueChange(float currentOffset)
{
	if (ItemCount == 0)
	{
		return;
	}
	if (canRefresh == true && !IsScrolling(currentOffset))
	{
		OnTouchStart();
		canRefresh = false;
	}
	if (currentOffset > offsetY)
	{
		offsetY = currentOffset;
		Forward();
	}
	else if (currentOffset < offsetY)
	{
		offsetY = currentOffset;
		Backward();
	}
	lastOffset = currentOffset;
}

void UWaterfallScrollView::Initiate()
{
	offsetY = 0;
	topBoundary = 0;
	recycleItemList.Empty();
	logicIndexMap.Empty();
	recycleIndexMap.Empty();
}

void UWaterfallScrollView::Fill()
{
	if (TemplateWidget.Get() == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("No Template!!"));
		return;
	}
	ItemCount = ItemCount > 0 ? ItemCount : 0;	
	if (ItemCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item count 0"));
		return;
	}
	ColumnCount = ColumnCount > 0 ? ColumnCount : 1;
	if (logicIndexMap.Num() == 0) // do not initial
	{
		visibleItemList.SetNum(ColumnCount);
		for (int i = 0; i < ColumnCount; i++)
		{
			logicIndexMap.Add(i, {});
			recycleIndexMap.Add(i, {});
			visibleItemList[i].Empty();
		}
	}
	FVector2D panelSize = CastChecked<UCanvasPanelSlot>(Slot)->GetSize();
	bottomBoundary = topBoundary + panelSize.Y;
	Forward();
}

void UWaterfallScrollView::RefreshLayout(int index)
{
	int column = 0;
	int startIndex = 0;
	bool outOfBounds = true;
	for (int i = 0; i < ColumnCount; i++)
	{
		int arrNum = logicIndexMap[i].Num();
		for (int j = 0; j < arrNum - 1; j++)
		{
			if (logicIndexMap[i][j] == index)
			{
				outOfBounds = false;
				column = i;
				startIndex = j;
				break;
			}
		}
	}
	if (outOfBounds)
	{
		return;
	}
	float currentPosition = 0;
	UWidget* startItem = visibleItemList[column][startIndex];
	startItem->ForceLayoutPrepass();
	if (startItem == nullptr)
	{
		return;
	}
	currentPosition = GetItemPosition(startItem) + GetItemHeight(startItem);
	int total = visibleItemList[column].Num();
	for (int i = startIndex + 1; i < total; i++)
	{
		UWidget* item = visibleItemList[column][i];
		UCanvasPanelSlot* slot = CastChecked<UCanvasPanelSlot>(item->Slot);
		slot->SetPosition(FVector2D(column * GetItemWidth(item), currentPosition));
		currentPosition += GetItemHeight(item);
	}
	Forward();
}

void UWaterfallScrollView::Forward()
{
	int column;
	while ((column = TopColumnToRecycleItem()) != -1)
	{
		RecycleTopItem(column);
		ForceLayoutPrepass();
	}
	while ((column = BottomColumnToAddItem()) != -1)
	{
		AddBottomItem(column);
		ForceLayoutPrepass();
	}
}

void UWaterfallScrollView::Backward()
{
	int column;
	while ((column = BottomColumnToRecycleItem()) != -1)
	{
		RecycleBottomItem(column);
		ForceLayoutPrepass();
	}
	while ((column = TopColumnToAddItem()) != -1)
	{
		AddTopItem(column);
		ForceLayoutPrepass();
	}
}

UWidget* UWaterfallScrollView::GetTopItem(int column)
{
	int num = visibleItemList[column].Num();
	return (num == 0) ? nullptr : visibleItemList[column][0];
}

UWidget* UWaterfallScrollView::GetBottomItem(int column)
{
	int num = visibleItemList[column].Num();
	return (num == 0) ? nullptr : visibleItemList[column][num - 1];
}

int UWaterfallScrollView::TopColumnToAddItem()
{
	if (GetMinLogicIndex() == 1)
	{
		return -1;
	}
	float maxTopY = 0;
	int index = -1;
	for (int i = ColumnCount - 1; i >= 0; i--)
	{
		if (logicIndexMap[i].Num() > 0 && logicIndexMap[i][0] == recycleIndexMap[i][0])
		{
			continue;
		}
		UWidget* topItem = GetTopItem(i);
		if (topItem == nullptr)
		{
			continue;
		}
		float curTopY = GetItemPosition(topItem);
		if (maxTopY == 0 || curTopY > maxTopY)
		{
			maxTopY = curTopY;
			index = i;
		}
	}
	if (maxTopY > topBoundary + offsetY)
	{
		return index;
	}
	return -1;
}

int UWaterfallScrollView::TopColumnToRecycleItem()
{
	if (GetTotalNum() == 0)
	{
		return -1;
	}
	float minTopY = 0;
	int index = -1;
	for (int i = ColumnCount - 1; i >= 0; i--)
	{
		UWidget* topItem = GetTopItem(i);
		if (topItem == nullptr)
		{
			continue;
		}
		float curTopY = (GetItemPosition(topItem) + GetItemHeight(topItem));
		if (minTopY == 0 || curTopY < minTopY)
		{
			minTopY = curTopY;
			index = i;
		}
	}
	if (minTopY < topBoundary + offsetY)
	{
		return index;
	}
	return -1;
}

int UWaterfallScrollView::BottomColumnToRecycleItem()
{
	if (GetTotalNum() == 0)
	{
		return -1;
	}
	float maxBottomY = 0;
	int index = -1;
	for (int i = ColumnCount - 1; i >= 0; i--)
	{
		UWidget* bottomItem = GetBottomItem(i);
		if (bottomItem == nullptr)
		{
			continue;
		}
		float curBottomY = GetItemPosition(bottomItem);
		if (maxBottomY == 0 || curBottomY > maxBottomY)
		{
			maxBottomY = curBottomY;
			index = i;
		}
	}
	if (maxBottomY > bottomBoundary + offsetY)
	{
		return index;
	}
	return -1;
}

int UWaterfallScrollView::BottomColumnToAddItem()
{
	if (ItemCount <= GetTotalNum())
	{
		return -1;
	}
	float minBottomY = 0;
	int index = -1;
	for (int i = 0; i < ColumnCount; i++)
	{
		UWidget* bottomItem = GetBottomItem(i);
		if (bottomItem == nullptr)
		{
			index = i;
			break;
		}
		float curBottomY = (GetItemPosition(bottomItem) + GetItemHeight(bottomItem));
		if (minBottomY == 0 || curBottomY < minBottomY)
		{
			minBottomY = curBottomY;
			index = i;
		}
	}
	if (minBottomY < bottomBoundary + offsetY)
	{
		return index;
	}
	return -1;
}


void UWaterfallScrollView::AddBottomItem(int column)
{
	UWidget* container = GetContainer();
	UWidget* item = GetItem();
    if (item == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create item"));
        return;
    }
	CastChecked<UCanvasPanel>(container)->AddChildToCanvas(item);
	int lastIndex = GetLastIndex() + 1;
	if (onFillItem.isFunction())
	{
		onFillItem.call(item, lastIndex);
	}
	logicIndexMap[column].Add(lastIndex);
	recycleIndexMap[column].AddUnique(lastIndex);
	item->ForceLayoutPrepass();
	UWidget* bottomItem = GetBottomItem(column);
	float positionY = (bottomItem == nullptr ? 0 : GetItemPosition(bottomItem) + GetItemHeight(bottomItem));
	UCanvasPanelSlot* slot = CastChecked<UCanvasPanelSlot>(item->Slot);
	slot->SetPosition(FVector2D(column * GetItemWidth(item), positionY));
	slot->SetSize(FVector2D(GetItemWidth(item), GetItemHeight(item)));
	item->ForceLayoutPrepass();
	int curIndex = GetLastIndex() + 1;
	visibleItemList[column].Add(item);
}

void UWaterfallScrollView::AddTopItem(int column)
{
	UWidget* container = GetContainer();
	UWidget* item = GetItem();
    if (item == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create item"));
        return;
    }
	UWidget* topItem = GetTopItem(column);
	float topItemPosition = GetItemPosition(topItem);
	CastChecked<UCanvasPanel>(container)->AddChildToCanvas(item);
	//int index = GetMinLogicIndex() - 1;
	int curTopIndex = logicIndexMap[column][0];
	int topPos = recycleIndexMap[column].Find(curTopIndex);
	int index = recycleIndexMap[column][topPos - 1];
	if (onFillItem.isFunction())
	{
		onFillItem.call(item, index);
	}
	item->ForceLayoutPrepass(); //refresh item height immediately
	UCanvasPanelSlot* slot = CastChecked<UCanvasPanelSlot>(item->Slot);
	float positionY = topItemPosition - GetItemHeight(item);
	slot->SetPosition(FVector2D(column * GetItemWidth(item), positionY));
	slot->SetSize(FVector2D(GetItemWidth(item), GetItemHeight(item)));
	item->ForceLayoutPrepass(); //update item position immediately
	logicIndexMap[column].Insert(index, 0);
	visibleItemList[column].Insert(item, 0);
}

void UWaterfallScrollView::RecycleTopItem(int column)
{
	UWidget* topItem = GetTopItem(column);
	recycleItemList.Add(topItem);
	visibleItemList[column].RemoveAt(0);
	int index = logicIndexMap[column][0];
	if (onRecycleItem.isFunction())
	{
		onRecycleItem.call(topItem, index);
	}
	logicIndexMap[column].RemoveAt(0);
}

void UWaterfallScrollView::RecycleBottomItem(int column)
{
	UWidget* bottomItem = GetBottomItem(column);
	recycleItemList.Add(bottomItem);
	visibleItemList[column].RemoveAt(visibleItemList[column].Num() - 1);
	int removeIndex = logicIndexMap[column].Num() - 1;
	int recycleIndex = logicIndexMap[column][removeIndex];
	if (onRecycleItem.isFunction())
	{
		onRecycleItem.call(bottomItem, recycleIndex);
	}
	logicIndexMap[column].RemoveAt(removeIndex);
}

int UWaterfallScrollView::GetLastIndex()
{
	int index = 0;
	for (int i = 0; i < ColumnCount; i++)
	{
		if (logicIndexMap[i].Num() == 0)
		{
			continue;
		}
		int maxIndex = logicIndexMap[i].Last();
		if (index == 0 || maxIndex > index)
		{
			index = maxIndex;
		}
	}
	return index;
}

int UWaterfallScrollView::GetFirstIndex()
{
	return GetLastIndex() - GetVisibleItemNum() + 1;
}

int UWaterfallScrollView::GetVisibleItemNum()
{
	int total = 0;
	for (int i = 0; i < ColumnCount; i++)
	{
		total += visibleItemList.Num();
	}
	return total;
}

int UWaterfallScrollView::GetTotalNum()
{
	return GetLastIndex();
}

// find justify min position to insert
int UWaterfallScrollView::GetMinLogicIndex()
{
	int index = 0;
	TArray<int> arr = {};
	for (int i = 0; i < ColumnCount; i++)
	{
		for (int j = 0, num = logicIndexMap[i].Num(); j < num; j++)
		{
			arr.Add(logicIndexMap[i][j]);
		}
	}
	arr.Sort();
	for (int i = arr.Num() - 1; i >= 0; i--)
	{
		int target = arr[i];
		if (index == 0 || target == index - 1)
		{
			index = target;
		}
		else // find dispersed
		{
			break;
		}
	}
	return index;
}

UWidget* UWaterfallScrollView::GetItem()
{
	UWidget* item;
	if (recycleItemList.Num() == 0)
	{
        
#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
        UGameInstance* GameInstance = nullptr;
#if WITH_EDITOR
        UUnrealEdEngine* engine = Cast<UUnrealEdEngine>(GEngine);
        if (engine && engine->PlayWorld) GameInstance = engine->PlayWorld->GetGameInstance();
#else
        UGameEngine* engine = Cast<UGameEngine>(GEngine);
        if (engine) GameInstance = engine->GameInstance;
#endif
        if (!GameInstance) return nullptr;
        item = CreateWidget<UWidget>(GameInstance, TemplateWidget.Get());
#else
        item = CreateWidget(this, TemplateWidget.Get());
#endif
	}
	else
	{
		item = recycleItemList[0];
		recycleItemList.RemoveAt(0);
	}
	return item;
}

float UWaterfallScrollView::GetItemPosition(UWidget* item)
{
	UCanvasPanelSlot* slot = Cast<UCanvasPanelSlot>(item->Slot);
	return slot->GetPosition().Y;
}

float UWaterfallScrollView::GetItemHeight(UWidget* item)
{
	return item->GetDesiredSize().Y;
}

float UWaterfallScrollView::GetItemWidth(UWidget* item)
{
	return item->GetDesiredSize().X;
}

void UWaterfallScrollView::OnTouchStart()
{
	canRefresh = false;
	UWidget* container = GetContainer();
	touchStartOffset = USlateBlueprintLibrary::LocalToAbsolute(container->GetCachedGeometry(), FVector2D(0, 0)).Y;
}

void UWaterfallScrollView::OnTouchEnd()
{
	canRefresh = true;
	if (touchStartOffset == 0)
	{
		return;
	}
	UWidget* container = GetContainer();
	touchEndOffset = USlateBlueprintLibrary::LocalToAbsolute(container->GetCachedGeometry(), FVector2D(0, 0)).Y;
	if (ScrollView->GetScrollOffset() == 0 && touchEndOffset - touchStartOffset > REFRESH_SENSITIVITY)
	{
		onReachTop.Broadcast();
		return;
	}
	if (FMath::Abs(ScrollView->GetScrollOffset() + bottomBoundary - container->GetDesiredSize().Y) < LOADMORE_OFFSET_RANGE && touchStartOffset - touchEndOffset > REFRESH_SENSITIVITY)
	{
		onReachBottom.Broadcast();
		return;
	}
}

UWidget* UWaterfallScrollView::GetContainer()
{
	int childCount = this->GetChildrenCount();
	for (int i = 0; i < childCount; i++)
	{
		UWidget* rootWidget = this->GetChildAt(i);
		if (rootWidget->GetName() == "Root")
		{
			UCanvasPanel* rootCanvasPanel = CastChecked<UCanvasPanel>(rootWidget);
			int count = rootCanvasPanel->GetChildrenCount();
			for (int j = 0; j < count; j++)
			{
				UWidget* contentWidget = rootCanvasPanel->GetChildAt(i);
				if (contentWidget->GetName() == "Container")
				{
					return contentWidget;
				}
			}
		}
	}
	return nullptr;
}

void UWaterfallScrollView::CreateSubContent()
{
	UWidgetTree* widgetTree = Cast<UWidgetTree>(GetOuter());
	UCanvasPanel* root = widgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	this->AddChild(root);
	UCanvasPanel* container = widgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Container"));
	root->AddChild(container);
	CastChecked<UCanvasPanelSlot>(container->Slot)->SetAutoSize(true);
}

void UWaterfallScrollView::Clear()
{
	if (ScrollView.IsValid())
	{
		ScrollView->ScrollToStart();
		if (logicIndexMap.Num() == 0)
		{
			return;
		}
		UCanvasPanel* container = CastChecked<UCanvasPanel>(GetContainer());
		container->ClearChildren();
		//for (int i = 0; i < ColumnCount; i++)
		//{
		//	int num = visibleItemList[i].Num();
		//	for (int j = 0; j < num; j++)
		//	{
		//		UWidget* item = visibleItemList[i][j];
		//		if (onRecycleItem.isFunction())
		//		{
		//			int index = logicIndexMap[i][j];
		//			onRecycleItem.call(item, index);
		//		}
		//	}
		//}
		Initiate();
	}
}

float UWaterfallScrollView::GetLastBottomPosition()
{
	float max = 0;
	for (int i = 0; i < ColumnCount; i++)
	{
		int num = visibleItemList[i].Num();
		UWidget* lastItem = visibleItemList[i][num - 1];
		float lastPos = GetItemPosition(lastItem) + GetItemHeight(lastItem);
		if (lastPos > max)
		{
			max = lastPos;
		}
	}
	return max;
}

bool UWaterfallScrollView::IsScrolling(float curOffset)
{
	return curOffset != lastOffset;
}

#undef LOCTEXT_NAMESPACE