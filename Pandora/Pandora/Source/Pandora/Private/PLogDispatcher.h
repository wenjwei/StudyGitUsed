#pragma once

#include "CoreMinimal.h"

DECLARE_DELEGATE_OneParam(FOnLogDispatchedDelegate, const FString&);

namespace pandora
{
	class PLogDispatcher
	{
	public:
		virtual ~PLogDispatcher() { };
		static PLogDispatcher& Get();
		void DispatchLog(const FString& msg);
		FOnLogDispatchedDelegate OnLogDispatchedDelegate;

	private:
		PLogDispatcher() { };
	};

};
