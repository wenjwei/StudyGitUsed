#pragma once

/*
* Interfaces provided for game
*/

#include "UnrealString.h"
#include "Map.h"
#include "Math/Vector2D.h"
#include "SharedPointer.h"
#include "Json.h"
#include "Runtime/CoreUObject/Public/UObject/ObjectMacros.h"


class UGameInstance;
class UUserWidget;
class UCanvasPanel;
class UImage;

UENUM()
enum EPandoraEnv
{
    Test            UMETA(DisplayName = "TestEnv"),
    Product         UMETA(DisplayName = "ProductiveEnv"),
    Alpha           UMETA(DisplayName = "AlphaEnv"),
};

#define NS_PANDORA pandora

namespace pandora
{
class PANDORA_API PPandora
{
public:
    virtual ~PPandora() { }

    /// main interfaces
    static PPandora & Get();
    void Init(UGameInstance *, bool enable, EPandoraEnv env = EPandoraEnv::Product);
    /*
    userdata = 
    {
        {TEXT("sOpenId"),       openid},                // openid
        {TEXT("sServiceType"),  servicetype},           // 业务代码
        {TEXT("sAppId"),        appid},                 // 游戏app唯一标识，qq wx不同
        {TEXT("sRoleId"),       roleid},                // 游戏角色id
        {TEXT("sPlatID"),       platid},                // 平台id: android = 1, ios = 0
        {TEXT("sAcountType"),   acctype},               // 玩家账户类型: "qq" or "wx"
        {TEXT("sArea"),         area},                  // 大区id
        {TEXT("sPartition"),    partition},             // 小区(游戏内区服)
        {TEXT("sAccessToken"),  access_token},          // 接入msdk返回的token
        {TEXT("sGameVer"),      gameversion},           // 游戏版本号
        {TEXT("sPayToken"),     pay_token},             // 支付token
        {TEXT("sChannelID"),    channel_id},            // 渠道id
        {TEXT("sQQInstalled"),  qq_installed},          // 预留字段，qq是否已安装，"0"为未安装，"1"为已安装
        {TEXT("sWXInstalled"),  wx_installed},          // 预留字段，wx是否已安装，"0"为未安装，"1"为已安装
        {TEXT("sExtend"),       extend_args},           // 预留字段，扩展参数
        {TEXT("sLanSuffix"),    lan_suffix},            // 多语言包后缀
    }
    */
    void SetUserData(const TMap<FString, FString> & userdata);
    void Close();
    void Do(const TMap<FString, FString> & cmd);
    void Do(const FString & cmd);
    void Do(TSharedPtr<FJsonObject> obj);

    /// misc
    int CallGame(const FString & cmd);
    UWorld * GetWorld();
    UGameInstance * GetGameInstance();
    void RefreshUserDataTokens();
    int GetSDKVersion();
    void SetSDKVersion(int);
    void SetSDKCoreDataPath(const FString& path);

    /// delegate defines
    typedef int(*CallGameDelegate)(const FString& cmd);
    typedef void(*PlaySoundDelegate)(const FString & soundid);
    typedef void(*GetAccountTokenDelegate)(TMap<FString, FString>& results);
    typedef void(*GetCurrencyDelegate)(TMap<FString, FString>& results);
    typedef void(*JumpDelegate)(const FString& jumpType, const FString& jumpContent);
    typedef void(*AddUserWidgetToGameDelegate)(UUserWidget * widget, const FString& panelName);
    typedef void(*RemoveUserWidgetFromGameDelegate)(UUserWidget * widget, const FString& panelName);
    typedef void(*ShowItemTipsDelegate)(UCanvasPanel* anchor, const FString & itemid);
    typedef void(*ShowItemIconDelegate)(UImage * image, const FString & itemid);
    typedef void(*ShowItemDelegate)(UCanvasPanel* anchor, const FString& itemid, int itemCnt);

    /// set delegate methods
    void SetCallGameDelegate(CallGameDelegate func);
    void SetPlaySoundDelegate(PlaySoundDelegate func);
    void SetGetAccountTokenDelegate(GetAccountTokenDelegate func);
    void SetGetCurrencyDelegate(GetCurrencyDelegate func);
    void SetJumpDelegate(JumpDelegate func);
    void SetAddUserWidgetToGameDelegate(AddUserWidgetToGameDelegate func);
    void SetRemoveUserWidgetFromGameDelegate(RemoveUserWidgetFromGameDelegate func);
    void SetShowItemTipsDelegate(ShowItemTipsDelegate func);
    void SetShowItemIconDelegate(ShowItemIconDelegate func);
    void SetShowItemDelegate(ShowItemDelegate func);


    /// call delegate methods;
    void PlaySound(const FString & id);
    void GetCurrency(TMap<FString, FString>& results);
    void Jump(const FString & type, const FString & content);
    void AddUserWidgetToGame(UUserWidget * widget, const FString & panelName);
    void RemoveUserWidgetFromGame(UUserWidget * widget, const FString & panelName);
    void ShowItemTips(UCanvasPanel* anchor, const FString & itemID);
    void ShowItemIcon(UImage * image, const FString & itemID);
    void ShowItem(UCanvasPanel* anchor, const FString& itemID, int itemCnt);

private:
    PPandora() { }

    CallGameDelegate _callGameDelegate = nullptr;
    PlaySoundDelegate _playSoundDelegate = nullptr;
    GetAccountTokenDelegate _getAccountTokenDelegate = nullptr;
    GetCurrencyDelegate _getCurrencyDelegate = nullptr;
    JumpDelegate _jumpDelegate = nullptr;
    AddUserWidgetToGameDelegate _addPanelToGameDelegate = nullptr;
    RemoveUserWidgetFromGameDelegate _removePanelFromGameDelegate = nullptr;
    ShowItemTipsDelegate _showItemTipsDelegate = nullptr;
    ShowItemIconDelegate _showItemIconDelegate = nullptr;
    ShowItemDelegate _showItemDelegate = nullptr;

    bool _isLoggedIn = false;
    bool _isEnable = false;
    EPandoraEnv _env = EPandoraEnv::Product;
    int _version = 1;

    UGameInstance * _gameInstance;
};
}
