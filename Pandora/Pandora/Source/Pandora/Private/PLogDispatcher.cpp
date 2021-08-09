#include "PLogDispatcher.h"

namespace pandora
{

	PLogDispatcher& PLogDispatcher::Get()
	{
		static PLogDispatcher _pLogDispatcherInstance;
		return _pLogDispatcherInstance;
	}

	void PLogDispatcher::DispatchLog(const FString& msg)
	{
		OnLogDispatchedDelegate.ExecuteIfBound(msg);
	}
}