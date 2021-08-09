# <span style="color:#5DADE2">**Unreal Engine 4 Pandora SDK**</span>

## <span style="color:#5DADE2">目录</span>
[TOC]

## <span style="color:#5DADE2">组成</span>
Pandora SDK 由 Pandora 及 PixUI 两部分插件组成
或仅由 Pandora 单个插件组成



## <span style="color:#5DADE2">接入</span>


### Pandora 接口调用流程
1. 调用PPandora::Init函数初始化Pandora模块，该步骤整个游戏生命周期仅需要调用一次
2. 设置各项委托，该步骤整个游戏生命周期仅需要设置一次
3. 用户数据准备好后，调用PPandora::SetUserData以启用Pandora SDK工作流程，该步骤在用户角色登录后调用
4. 角色退出或游戏结束，需要调用PPandora::Close接口



### PPandora 单例简介
该单例为Pandora SDK的运行时单例，所有游戏需要的接口均封装于该单例内，游戏对Pandora SDK的所有操作均应通过该单例实施



### UserData 简介
UserData为用户数据，其包含Pandora SDK所需要使用的一些用户角色相关的数据，以下示例为一组典型的用户数据示例。
**<span style="color:#E74C3C">需要注意的是，其键值由于历史原因有部分单词拼写并不正确，请忽视此类错误，严格按照以下示例设置键值</span>**

```c++
TMap<FString, FString> userdata = {
    {TEXT("sOpenId"),       openid},        // openid
    {TEXT("sServiceType"),  servicetype},   // 业务代码
    {TEXT("sAppId"),        appid},         // 游戏app唯一标识，qq wx不同
    {TEXT("sRoleId"),       roleid},        // 游戏角色id
    {TEXT("sPlatID"),       platid},        // 平台id: android = 1, ios = 0
    {TEXT("sAcountType"),   acctype},       // 玩家账户类型: "qq" or "wx"
    {TEXT("sArea"),         area},          // 大区id: 一般情况下 wx = 1, qq = 2, 视具体情况而定，请务必与服务端对齐
    {TEXT("sPartition"),    partition},     // 小区(游戏内区服)
    {TEXT("sAccessToken"),  access_token},  // 接入msdk返回的token
    {TEXT("sGameVer"),      gameversion},   // 游戏版本号
    {TEXT("sPayToken"),     pay_token},     // 支付token
    {TEXT("sChannelID"),    channel_id},    // 渠道id
    {TEXT("sQQInstalled"),  qq_installed},  // 预留字段，qq是否已安装，"0"为未安装，"1"为已安装
    {TEXT("sWXInstalled"),  wx_installed},  // 预留字段，wx是否已安装，"0"为未安装，"1"为已安装
    {TEXT("sExtend"),       extend_args},   // 预留字段，扩展参数
}
```
以上示例为一个较为完整的UserData示例，存在一些预留字段，以下列表所列字段为 Pandora SDK 能够正常运行所需要的基本数据段; 在调用SetUserData接口时如果传入数据不包含以下列表中的所有字段的话，SetUserData将会调用失败，并提示调用者缺少了何种字段。SetUserData函数简介位于[此处](####Pandora-SDK-用户数据设置接口)
```c++
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
```



### 主要接口

#### Pandora SDK 获取单例引用接口
```c++
static PPandora & PPandora::Get();
```
该接口用于获取PPandora单例的引用



#### Pandora SDK 初始化接口
```c++
void PPandora::Init(UGameInstance * instance, bool enable, EPandoraEnv env = EPandoraEnv::Product, bool isPixUIEnable = true);
```

##### 接口说明
该接口用于初始化 Pandora SDK 并设置一些基本信息

##### 参数说明
arg-0: 指向游戏运行时 UGameInstance 的指针
arg-1: 是否开启 Pandora SDK 组件，**<span style="color:#E74C3C">强烈建议将传入值设置为可热更的配置值</span>**
arg-2: Pandora SDK 连接到何种环境，一般情况下业务接入的Pandora环境仅分为测试环境与正式环境，```EPandoraEnv::Test```为测试环境，```EPandoraEnv::Product```为正式环境。 **<span style="color:#E74C3C">游戏链接到游戏的测试环境时，此处应该传入```EPandoraEnv::Test```，游戏链接到游戏的正式环境时，此处应传入```EPandoraEnv::Product```</span>**
arg-3: 是否启用PixUI组件，如提供的插件接口未提供此参数，则该插件版本为不包含PixUI的插件版本，忽视此参数即可



##### **注意事项**
**调用该接口仅设置一些基本开关，并不会启动 Pandora SDK 的 Lua 虚拟机**



#### Pandora SDK 用户数据设置接口
```c++
void PPandora::SetUserData(const TMap<FString, FString> & userdata);
```

##### 接口说明
该接口用于设置用户数据，用户数据简介位于[此处](###UserData-简介)

##### **注意事项**
**<span style="color:#E74C3C">调用该接口后，如果UserData正常设置，则SDK会自动启动其正常工作流程，该接口应于角色登录后调用</span>**



#### Pandora SDK 协议发送接口
```c++
void PPandora::Do(const TMap<FString, FString> & cmd);
void PPandora::Do(const FString & cmd);
void PPandora::Do(TSharedPtr<FJsonObject> obj);
```

##### 接口说明
该组接口为游戏向 Pandora SDK Lua 层发送协议时使用的接口，提供三种接口形式，即
1. 接受一个字符型映射表，接口会将该字典转换为 Json 字串的形式后将数据发送到 Pandora SDK Lua 层
2. 接受一个 Json 字符串，该字符串会直接发送到 Pandora SDK Lua 层
3. 接受一个 ```FJsonObject``` 型的共享指针，接口会将该 ```FJsonObject``` 序列化为 Json 字串后将数据发送到 Pandora SDK Lua 层



#### Pandora SDK 关闭接口
```c++
void PPandora::Close();
```

##### 接口说明
该接口用于手动结束 Pandora SDK 当前的工作流程

##### 注意事项
接口调用后 Pandora SDK 的虚拟机会被关闭，Pandora SDK 将进入静默状态，该接口应于角色退出或游戏关闭时调用



#### Pandora SDK 热更新文件设置接口
```c++
void PPandora::SetSDKCoreDataPath(const FString& path);
```

##### 接口说明
该接口用于在必要时设置SDK核心逻辑数据读取路径，以线上修复SDK核心逻辑的错误。
热更新文件由Pandora方提供，依赖游戏的热更新能力部署
接口接受一个字符串，该字符串应为热更新文件的资源路径。
当接口被调用，并传入非空字符串时，SDK将会尝试使用引擎接口```FFileHelper::LoadFileToArray```，以传入字符串作为路径，读取SDK核心逻辑数据。
当数据读取失败，SDK将使用```PDefaultLuaCoreData.h```文件中定义的数据作为核心逻辑数据。
当接口没有被调用，或传入了空字符串时，SDK将使用```PDefaultLuaCoreData.h```文件中定义的数据作为核心逻辑数据。
该接口如需调用，则应在```PPandora::SetUserData```之前调用



### 可选接口

#### Pandora SDK 版本设置接口
```c++
void PPandora::SetSDKVersion(int version);
```

##### 接口说明
该接口用于设置 Pandora SDK 版本号，若不调用则版本号默认为1；该接口是为某些特殊情况预留的，正常情况在SetUserData前调用该接口并传入**1**即可，**<span style="color:#E74C3C">强烈建议将该值设置为可配置值</span>**



### 主要委托设置接口

#### Pandora->Game 协议委托
```c++
typedef int(*CallGameDelegate)(const FString& msg);
void PPandora::SetCallGameDelegate(CallGameDelegate func);
```
##### 委托说明
该委托是必须设置的委托，所有Pandora发往的json串协议均由该委托处理



#### 跳转请求委托
```c++
typedef void(*JumpDelegate)(const FString& jumpType, const FString& jumpContent);
void PPandora::SetJumpDelegate(JumpDelegate func);
```
##### 委托说明
该委托是必须设置的委托，所有Pandora的跳转请求(H5, 游戏内界面等)均由该委托处理
Pandora面板支持的四种跳转参数配置：
###### 1).跳转网址(外部浏览器):Url:http://www.qq.com  **(个别项目没有该功能可以忽略)**
```
args:
{
"jumpType":"Url",
"jumpContent":"http://www.qq.com"
}
```
###### 2).跳转网址(内嵌浏览器):pandoraOpenUrl:http://www.qq.com
当拍脸活动中配置的跳转类型为跳转H5链接时，潘多拉会对跳转委托传入以下参数，以通知游戏通过MSDK内置浏览器打开H5链接。
```
args: 
{
"jumpType":"pandoraOpenUrl",
"jumpContent":"http://www.qq.gamecenter.com/wmsj/index.html"
}
```
###### 3).跳转游戏界面：pandoraGoSystem:jumpId
当拍脸活动中配置的跳转类型为跳转游戏界面时，潘多拉会对跳转委托传入以下参数，以通知游戏打开对应界面，content参数需要跟游戏对接。 
```
args: 
{
"jumpType":"pandoraGoSystem",
"jumpContent":"store"
}
```
###### 4).跳转Pandora界面: pandoraGoPandora:jumpId
当拍脸活动中配置的跳转类型为跳转潘多拉时，潘多拉会对跳转委托传入以下参数，游戏收到后给潘多拉发送对应活动的open消息。content参数一般为目标活动的活动名。 
```
args: 
{
"jumpType":"pandoraGoPandora",
"jumpContent":"pop"
}
```


#### 面板显示委托
```c++
typedef void(*AddUserWidgetToGameDelegate)(UUserWidget * widget, const FString& panelName);
void PPandora::SetAddUserWidgetToGameDelegate(AddUserWidgetToGameDelegate func);
```
##### 委托说明
该委托是必须设置的委托，所有Pandora活动界面均由该委托加入到游戏viewport中(或某个Canvas下，具体做法由游戏侧决定)
其中参数panelName用于指明面板身份，游戏可根据panelName决定如何处理widget
此处delegate实现建议游戏将widget连同游戏UI一起管理起来，方便后续pandora方利用游戏已有资源(例如Tips，提示框等)，以减少双方对接工作量



#### 面板回收委托
```c++
typedef void(*RemoveUserWidgetFromGameDelegate)(UUserWidget * widget, const FString& panelName);
void PPandora::SetRemoveUserWidgetFromGameDelegate(RemoveUserWidgetFromGameDelegate func);
```
##### 委托说明
该委托是必须设置的委托，所有显示中的Pandora活动界面均由该委托从视图中移除
panelName用于指明面板身份，游戏可根据panelName决定如何处理widget



### 可选委托设置接口

#### 音效播放委托
```c++
typedef void(*PlaySoundDelegate)(const FString & soundID);
void PPandora::SetPlaySoundDelegate(PlaySoundDelegate func);
```
##### 委托说明
该委托为可选委托，当Pandora需要播放游戏音效时将通过该接口请求播放



#### Token刷新委托
```c++
typedef void(*GetAccountTokenDelegate)(TMap<FString, FString>& results);
void PPandora::SetGetAccountTokenDelegate(GetAccountTokenDelegate func);
```
##### 委托说明
该委托为可选委托，某些情况下Pandora可能需要刷新PayToken与AccessToken，此种情况下Pandora将通过该委托刷新. results 为键值对，键值请对照UserData部分内容，UserData简介位于[此处](###UserData-简介)



#### 货币信息获取委托
```c++
typedef void(*GetCurrencyDelegate)(TMap<FString, FString>& results);
void PPandora::SetGetCurrencyDelegate(GetCurrencyDelegate func);
```
##### 委托说明
该委托为可选委托，当Pandora需要获取游戏货币信息时将通过该委托获取. results 为键值对，key为货币ID，value为对应货币拥有量



#### 显示物品Tips委托
```c++
typedef void(*ShowItemTipsDelegate)(const FVector2D & position, const FString & itemID);
void PPandora::SetShowItemTipsDelegate(ShowItemTipsDelegate func);
```
##### 委托说明
该委托为可选委托，当Pandora需要显示游戏物品Tips时将通过该委托请求显示



#### 显示物品图标委托
```c++
typedef void(*ShowItemIconDelegate)(UImage * image, const FString & itemID);
void PPandora::SetShowItemIconDelegate(ShowItemIconDelegate func);
```
##### 委托说明
该委托为可选委托，当Pandora需要在某个UImage上显示游戏物品图标时，将通过该委托将物品图标图像数据设置到UImage上


#### 显示物品信息图标
```c++
typedef void(*ShowItemDelegate)(UCanvasPanel * anchor, const FString& itemid, int itemCnt);
void PPandora::SetShowItemDelegate(ShowItemDelegate func);
```
#### 委托说明
该委托为可选委托，当Pandora需要显示较为完整的物品信息时（包含品质框数量等信息），将通过该委托请求游戏创建一个UUserWidget并挂载到anchor上

