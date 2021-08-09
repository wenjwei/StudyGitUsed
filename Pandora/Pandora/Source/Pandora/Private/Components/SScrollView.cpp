#include "SScrollView.h"

FReply SScrollView::OnTouchStarted(const FGeometry &MyGeometry, const FPointerEvent &InTouchEvent)
{
	onTouchStartDelegate.ExecuteIfBound();
	return SScrollBox::OnTouchStarted(MyGeometry, InTouchEvent);
}

FReply SScrollView::OnTouchEnded(const FGeometry &MyGeometry, const FPointerEvent &InTouchEvent)
{
	onTouchEndDelegate.ExecuteIfBound();
	return SScrollBox::OnTouchEnded(MyGeometry, InTouchEvent);
}
