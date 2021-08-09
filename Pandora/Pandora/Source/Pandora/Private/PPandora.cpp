
#include "PPandora.h"
#include "PLuaStateMgr.h"
#include "PLog.h"
#include "PLuaFileSystem.h"
#include "PCppUtilities.h"
#include "PSDKLuaCoreMgr.h"

#include "Runtime/Json/Public/Json.h"
#include "PDRLuaState.h"
#include "PDRLuaObject.h"
#include "PDRLuaVar.h"
#include "Engine.h"
#include "UMG/Public/Components/Image.h"
#include "Runtime/Launch/Resources/Version.h"

#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
#include "Image.h"
#else
#include "UMG/Public/Components/Image.h"
#endif

namespace pandora
{
PPandora & PPandora::Get()
{
    static PPandora ___ppandoraInstance;
    return ___ppandoraInstance;
}

void PPandora::Init(UGameInstance * instance, bool enable, EPandoraEnv env)
{
    _gameInstance = instance;
    _env = env;
    _isEnable = enable;
    PPaths::SetupPaths();
    PLog::SetupLogPath();
}

static TArray<FString> UserDataArgs = {
    TEXT("sOpenId"),
    TEXT("sAppId"),
    TEXT("sRoleId"),
    TEXT("sPlatID"),
    TEXT("sAcountType"),
    TEXT("sArea"),
    TEXT("sPartition"),
    TEXT("sAccessToken"),
    TEXT("sGameVer"),
    TEXT("sPayToken"),
    TEXT("sChannelID"),
};
inline bool IsUserDataValid(const TMap<FString, FString> & userdata)
{
    TArray<FString> missArgs;
    for (int i = 0; i < UserDataArgs.Num(); ++i)
    {
        if (!userdata.Contains(UserDataArgs[i]))
            missArgs.Add(UserDataArgs[i]);
    }
    if (missArgs.Num() > 0)
    {
        FString errmsg = TEXT("the following args are missing: ");
        for (int i = 0; i < missArgs.Num(); ++i)
        {
            errmsg.Append(missArgs[i]);
            if (i != missArgs.Num())
                errmsg.Append(TEXT(", "));
        }
        PLog::LogError(errmsg);
        return false;
    }
    return true;
}

void PPandora::SetUserData(const TMap<FString, FString> & userdata)
{
    if (!_isEnable)
    {
        return;
    }

    if (!IsUserDataValid(userdata))
    {
        return;
    }

    PLog::SetEnabled(true);

    if (PSDKLuaCoreMgr::Get().Reload() != 0)
    {
        return;
    }


    if (!PLuaStateMgr::GetMgr().IsInitialized())
    {
        int err = PLuaStateMgr::GetMgr().Init(_gameInstance);
        if (err != 0)
        {
            FString msg = TEXT("LuaStateMgr::Init exec failed, err: ") + FString::FromInt(err);
            PLog::LogError(msg);
            return;
        }
    }

    if (_isLoggedIn)
    {
        PLog::LogWarning(TEXT("Pandora is already logged in"));
        return;
    }

    FString jsonstr = PCppUtilities::StringMapToJsonString(userdata);
    PLuaStateMgr::GetMgr().GetLuaState().call("pandora.LuaInterface.SetSDKVersion", _version);
    PLuaStateMgr::GetMgr().GetLuaState().call("pandora.LuaInterface.SetEnvironment", _env);
    PLuaStateMgr::GetMgr().GetLuaState().call("pandora.LuaInterface.SetUserData", jsonstr);
    _isLoggedIn = true;
}

void PPandora::Close()
{
    if (!_isEnable)
    {
        PLog::LogWarning(TEXT("Pandora is not enabled"));
        return;
    }
    if (!PLuaStateMgr::GetMgr().IsInitialized())
    {
        PLog::LogWarning(TEXT("LuaStateMgr is not initialized"));
        return;
    }

    if (!_isLoggedIn)
    {
        PLog::LogWarning(TEXT("Pandora is not log in yet"));
        return;
    }

    _isLoggedIn = false;
    PLog::LogWarning(TEXT("Pandora SDK Closing"));
    PLuaStateMgr::GetMgr().GetLuaState().call("pandora.LuaInterface.DispatchPandoraExitEvent");
    PLuaStateMgr::GetMgr().Close();
    PSDKLuaCoreMgr::Get().Clear();
    PLog::Clear();
}

void PPandora::Do(const TMap<FString, FString> & cmd)
{
    FString jsonstr = PCppUtilities::StringMapToJsonString(cmd);
    Do(jsonstr);
}

void PPandora::Do(const FString & cmd)
{
    if (!_isEnable)
    {
        PLog::LogError(TEXT("PPandora::Do MUST NOT be called when pandora is not enabled"));
        return;
    }

    if (!PLuaStateMgr::GetMgr().IsInitialized())
    {
        PLog::LogError(TEXT("PPandora::Do must call after PPandora::Init is called"));
        return;
    }

    PLuaStateMgr::GetMgr().GetLuaState().call("pandora.LuaInterface.DispatchGameCommand", cmd);
}

void PPandora::Do(TSharedPtr<FJsonObject> obj)
{
    FString cmd;
    TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&cmd);
    FJsonSerializer::Serialize(obj.ToSharedRef(), writer);
    Do(cmd);
}

int PPandora::CallGame(const FString & cmd)
{
    if (_callGameDelegate == nullptr)
        return -1;

    return _callGameDelegate(cmd);
}

UWorld * PPandora::GetWorld()
{
    if (_gameInstance == nullptr)
        return nullptr;
    return _gameInstance->GetWorld();
}

UGameInstance * PPandora::GetGameInstance()
{
    return _gameInstance;
}

void PPandora::RefreshUserDataTokens()
{
    if (_getAccountTokenDelegate == nullptr)
    {
        PLog::LogWarning(TEXT("GetAccountTokenDelegate is not set"));
        return;
    }
    TMap<FString, FString> tokens;
    (*_getAccountTokenDelegate) (tokens);

    FString jsonTokens = PCppUtilities::StringMapToJsonString(tokens);
    PLuaStateMgr::GetMgr().GetLuaState().call("pandora.LuaInterface.RefreshUserDataTokens", jsonTokens);
}

int PPandora::GetSDKVersion()
{
    return _version;
}

void PPandora::SetSDKVersion(int version)
{
    _version = version;
}

void PPandora::SetSDKCoreDataPath(const FString& path)
{
    PSDKLuaCoreMgr::Get().SetOverrideDataFilePath(path);
}

void PPandora::SetCallGameDelegate(PPandora::CallGameDelegate func)
{
    _callGameDelegate = func;
}

void PPandora::SetPlaySoundDelegate(PPandora::PlaySoundDelegate func)
{
    _playSoundDelegate = func;
}

void PPandora::SetGetAccountTokenDelegate(PPandora::GetAccountTokenDelegate func)
{
    _getAccountTokenDelegate = func;
}

void PPandora::SetGetCurrencyDelegate(PPandora::GetCurrencyDelegate func)
{
    _getCurrencyDelegate = func;
}

void PPandora::SetJumpDelegate(PPandora::JumpDelegate func)
{
    _jumpDelegate = func;
}

void PPandora::SetAddUserWidgetToGameDelegate(PPandora::AddUserWidgetToGameDelegate func)
{
    _addPanelToGameDelegate = func;
}

void PPandora::SetRemoveUserWidgetFromGameDelegate(PPandora::RemoveUserWidgetFromGameDelegate func)
{
    _removePanelFromGameDelegate = func;
}

void PPandora::SetShowItemTipsDelegate(PPandora::ShowItemTipsDelegate func)
{
    _showItemTipsDelegate = func;
}

void PPandora::SetShowItemIconDelegate(PPandora::ShowItemIconDelegate func)
{
    _showItemIconDelegate = func;
}

void PPandora::SetShowItemDelegate(PPandora::ShowItemDelegate func)
{
    _showItemDelegate = func;
}

void PPandora::PlaySound(const FString & id)
{
    if (_playSoundDelegate == nullptr)
    {
        PLog::LogWarning(TEXT("PlaySoundDelegate is not set"));
        return;
    }
    (*_playSoundDelegate) (id);
}

void PPandora::GetCurrency(TMap<FString, FString>& results)
{
    if (_getCurrencyDelegate == nullptr)
    {
        PLog::LogWarning(TEXT("GetCurrencyDelegate is not set"));
        return;
    }
    (*_getCurrencyDelegate) (results);
}

void PPandora::Jump(const FString & type, const FString & content)
{
    if (_jumpDelegate == nullptr)
    {
        PLog::LogWarning(TEXT("JumpDelegate is not set"));
        return;
    }
    (*_jumpDelegate) (type, content);
}

void PPandora::AddUserWidgetToGame(UUserWidget * widget, const FString & panelName)
{
    if (widget == nullptr)
    {
        PLog::LogWarning(TEXT("Widget is null"));
        return;
    }
    if (!widget->IsValidLowLevel())
    {
        PLog::LogWarning(TEXT("Widget object is not valid"));
        return;
    }
    if (_addPanelToGameDelegate == nullptr)
    {
        PLog::LogWarning(TEXT("AddUserWidgetToGameDelegate is not set"));
        return;
    }
    (*_addPanelToGameDelegate) (widget, panelName);
}

void PPandora::RemoveUserWidgetFromGame(UUserWidget * widget, const FString& panelName)
{
    if (widget == nullptr)
    {
        PLog::LogWarning(TEXT("Widget is null"));
        return;
    }
    if (!widget->IsValidLowLevel())
    {
        PLog::LogWarning(TEXT("Widget object is not valid"));
        return;
    }
    if (_removePanelFromGameDelegate == nullptr)
    {
        PLog::LogWarning(TEXT("RemoveUserWidgetFromGameDelegate is not set"));
        return;
    }
    (*_removePanelFromGameDelegate) (widget, panelName);
}

void PPandora::ShowItemTips(UCanvasPanel* anchor, const FString & itemID)
{
    if (_showItemTipsDelegate == nullptr)
    {
        PLog::LogWarning(TEXT("ShowItemTipsDelegate is not set"));
        return;
    }
    (*_showItemTipsDelegate) (anchor, itemID);
}

void PPandora::ShowItemIcon(UImage * image, const FString & itemID)
{
    if (image == nullptr)
    {
        PLog::LogWarning(TEXT("Image is null"));
        return;
    }
    if (!image->IsValidLowLevel())
    {
        PLog::LogWarning(TEXT("Image object is not valid"));
        return;
    }
    if (_showItemIconDelegate == nullptr)
    {
        PLog::LogWarning(TEXT("ShowItemIconDelegate is not set"));
        return;
    }
    (*_showItemIconDelegate) (image, itemID);
}

void PPandora::ShowItem(UCanvasPanel* anchor, const FString& itemID, int itemCnt)
{
    if (_showItemDelegate == nullptr)
    {
        PLog::LogWarning(TEXT("CreateItemUMGDelegate is not set"));
        return;
    }

    return (*_showItemDelegate) (anchor, itemID, itemCnt);
}

}

