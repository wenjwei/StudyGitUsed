//#define USING_NGUI
#define USING_UGUI
using System;
using System.Collections.Generic;
using System.Text;
using UnityEngine;

namespace com.tencent.pandora
{
    public partial class Pandora
    {
        #region singleton instance
        private static Pandora _instance;
        public static Pandora Instance
        {
            get
            {
                if (_instance == null)
                {
                    _instance = new Pandora();
                }
                return _instance;
            }
        }
        #endregion
        private bool _isInitialized = false;
        //游戏UI面板的参考分辨率
        private Vector2 _referenceResolution;
        //该节点是添加各种Pandora组件的节点
        private GameObject _gameObject;
        //该节点是Pandora面板默认的父节点
        private GameObject _panelRoot;
        private string _panelRootName;
        private UserData _userData = new UserData();
        private RemoteConfig _remoteConfig;

        private RemoteConfigBrokerSocket _remoteConfigBrokerSocket;
        private BrokerSocket _brokerSocket;
        private AtmSocket _atmSocket;
        private LogHook _logHook;

        private Action<string> _jsonGameCallback;
        private Action<Dictionary<string, string>> _gameCallback;
        //播放游戏中音乐的委托
        private Action<string> _playSoundDelegate;
        //获取最新Token的委托，返回游戏玩家帐号AccessToken和PayToken
        private Func<Dictionary<string, string>> _getAccountTokenDelegate;
        //获取货币委托，返回Key为货币名称，Value为货币数量的Dictionary
        private Func<Dictionary<string, string>> _getCurrencyDelegate;
        private Dictionary<string, string> _defaultCurrency;
        //跳转委托
        private Action<string, string> _jumpDelegate;
        /**注意ShowItem和ShowItemIcon的区别，ShowItem是显示整个道具格子的信息，包括icon，品质框等等，ShowItemIcon只是显示道具图标**/
        //显示物品格子委托，参数为挂载点GameObject，道具id，道具数量
        private Action<GameObject, uint, uint> _showItemDelegate;
        //显示Icon图标的委托，参数为挂载点GameObject和道具id
        private Action<GameObject, uint> _showItemIconDelegate;
        //显示道具Tips委托，参数为挂载点GameObject和道具id
        private Action<GameObject, uint> _showItemTipsDelegate;

        private Dictionary<string, Font> _fontDict = new Dictionary<string, Font>();
        private PandoraStatusHelper _statusHelper;

        /// <summary>
        /// referenceResolution:游戏界面参考分辨率
        /// isProductEnvironment：连接Pandora正式或测试环境
        /// pandoraGoName：Pandora面板根节点
        /// </summary>
        /// <param name="referenceResolution"></param>
        /// <param name="isProductEnvironment"></param>
        /// <param name="pandoraGameObjectName"></param>
        public void Init(Vector2 referenceResolution, bool isProductEnvironment, string rootName)
        {
            if (_isInitialized == false)
            {
                _isInitialized = true;
                _referenceResolution = referenceResolution;
                _statusHelper = new PandoraStatusHelper();
                PandoraSettings.UseHttps = true;
                PandoraSettings.IsProductEnvironment = isProductEnvironment;
                PandoraSettings.ReadEnvironmentSetting();
                ErrorCodeConfig.Initialize();
                InitDelegate();
                CreatePandoraGameObject();
                _panelRoot = GameObject.Find(rootName);
                _panelRootName = rootName;
                LocalDirectoryHelper.Initialize();
                AssetManager.Initialize();
                PanelManager.Initialize();
                TextPartner.GetFont = GetFont;
                Logger.LogLevel = PandoraSettings.DEFAULT_LOG_LEVEL;
                Logger.LogInfo("<color=#00ffff>Pandora Init " + GetSystemInfo() + "</color>", PandoraSettings.SDKTag);
                _remoteConfigBrokerSocket = _gameObject.AddComponent<RemoteConfigBrokerSocket>();
                _remoteConfigBrokerSocket.AlternateIp1 = GetBrokerAlternateIp1();
                _remoteConfigBrokerSocket.AlternateIp2 = GetBrokerAlternateIp2();
                _remoteConfigBrokerSocket.Host = GetBrokerHost();
                _remoteConfigBrokerSocket.Port = GetBrokerPort();
                _atmSocket = _gameObject.AddComponent<AtmSocket>();
                _atmSocket.Connect(GetAtmHost(), GetAtmPort(), null);
            }
        }

        private string GetSystemInfo()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("DeviceModel="); sb.Append(SystemInfo.deviceModel);
            sb.Append("&DeviceName="); sb.Append(SystemInfo.deviceName);
            sb.Append("&DeviceType="); sb.Append(SystemInfo.deviceType);
            sb.Append("&OperatingSystem="); sb.Append(SystemInfo.operatingSystem);
            sb.Append("&ProcessorType="); sb.Append(SystemInfo.processorType);
            sb.Append("&SystemMemorySize="); sb.Append(SystemInfo.systemMemorySize);
            sb.Append("&GraphicsDeviceName="); sb.Append(SystemInfo.graphicsDeviceName);
            return sb.ToString();
        }

        private void CreatePandoraGameObject()
        {
            _gameObject = new GameObject("PandoraGameObject");
            _gameObject.AddComponent<LogHook>();
            GameObject.DontDestroyOnLoad(_gameObject);
        }

        /// <summary>
        /// 为Pandora面板指定特定的父节点
        /// </summary>
        /// <param name="panelName"></param>
        /// <param name="panelParent"></param>
        public void SetPanelParent(string panelName, GameObject panelParent)
        {
            PanelManager.SetPanelParent(panelName, panelParent);
        }

        public GameObject GetPanelRoot()
        {
            if (_panelRoot != null)
            {
                return _panelRoot;
            }

            if (!string.IsNullOrEmpty(_panelRootName))
            {
                _panelRoot = GameObject.Find(_panelRootName);
                if (_panelRoot != null)
                {
                    return _panelRoot;
                }
            }

            Logger.LogWarning("the root node game set for pandora is null，pandora default root node will be used instead",PandoraSettings.SDKTag);
#if USING_NGUI
            if (_gameObject.GetComponent<UIRoot>() == null)
            {
                UIRoot root = _gameObject.AddComponent<UIRoot>();
                root.scalingStyle = UIRoot.Scaling.Constrained;
                root.manualWidth = (int)_referenceResolution.x;
                root.manualHeight = (int)_referenceResolution.y;
                GameObject cameraGo = new GameObject("Camera");
                cameraGo.transform.SetParent(_gameObject.transform);
                Camera camera = cameraGo.AddComponent<Camera>();
                camera.cullingMask = LayerMask.GetMask("UI");
                camera.clearFlags = CameraClearFlags.Depth;
                camera.nearClipPlane = -100;
                camera.farClipPlane = 100;
                camera.orthographic = true;
                camera.orthographicSize = 1.0f;
                UICamera uiCamera = cameraGo.AddComponent<UICamera>();
                uiCamera.eventType = UICamera.EventType.UI_3D;
                uiCamera.eventReceiverMask = LayerMask.GetMask("UI");
            }
#endif
#if USING_UGUI
            if (_gameObject.GetComponent<Canvas>() == null)
            {
                Canvas canvas = _gameObject.AddComponent<Canvas>();
                canvas.renderMode = RenderMode.ScreenSpaceOverlay;
                UnityEngine.UI.CanvasScaler scaler = _gameObject.AddComponent<UnityEngine.UI.CanvasScaler>();
                scaler.uiScaleMode = UnityEngine.UI.CanvasScaler.ScaleMode.ScaleWithScreenSize;
                scaler.referenceResolution = _referenceResolution;
                _gameObject.AddComponent<UnityEngine.UI.GraphicRaycaster>();
            }
#endif
            return _gameObject;
        }

        public GameObject GetGameObject()
        {
            return _gameObject;
        }

        public void SetPlaySoundDelegate(Action<string> playSoundDelegate)
        {
            _playSoundDelegate = playSoundDelegate;
        }

        public void PlaySound(string id)
        {
            if (_playSoundDelegate != null)
            {
                _playSoundDelegate(id);
            }
        }

        /// <summary>
        /// 设置获取游戏内玩家最新登录态的委托
        /// 该委托返回Key为AccessToken和PayToken，Value为对应值的Dictionary
        /// </summary>
        /// <param name="getAccountTokenDelegate"></param>
        public void SetGetAccountTokenDelegate(Func<Dictionary<string, string>> getAccountTokenDelegate)
        {
            _getAccountTokenDelegate = getAccountTokenDelegate;
        }

        /// <summary>
        /// 设置获取游戏内玩家货币数量委托
        /// 该委托返回Key为货币名称(一级代币使用currency1作为key，二级代币适用currency2,以此类推)，Value为对应值的Dictionary
        /// </summary>
        /// <param name="getCurrencyDelegate"></param>
        public void SetGetCurrencyDelegate(Func<Dictionary<string, string>> getCurrencyDelegate)
        {
            _getCurrencyDelegate = getCurrencyDelegate;
        }

        public Dictionary<string, string> GetCurrency()
        {
            if (_getCurrencyDelegate != null)
            {
                return _getCurrencyDelegate();
            }
            if (_defaultCurrency == null)
            {
                _defaultCurrency = new Dictionary<string, string>();
#if UNITY_EDITOR
                _defaultCurrency["currency1"] = "-1";
                _defaultCurrency["currency2"] = "-1";
#endif
            }
            return _defaultCurrency;
        }

        /// <summary>
        /// 设置调整委托
        /// 跳转的具体类型和内容和解析和游戏开发讨论
        /// </summary>
        /// <param name="jumpDelegate"></param>
        public void SetJumpDelegate(Action<string, string> jumpDelegate)
        {
            _jumpDelegate = jumpDelegate;
        }

        public void Jump(string type, string content)
        {
            if (_jumpDelegate != null)
            {
                _jumpDelegate(type, content);
            }
        }

        /// <summary>
        /// 设置显示游戏内物品单元格委托
        /// 参数为挂载点GameObject，物品Id和物品数量
        /// </summary>
        /// <param name="showItemDelegate"></param>
        public void SetShowItemDelegate(Action<GameObject, uint, uint> showItemDelegate)
        {
            _showItemDelegate = showItemDelegate;
        }

        public void ShowItem(GameObject go, uint itemId, uint itemCount)
        {
            if (_showItemDelegate != null)
            {
                _showItemDelegate(go, itemId, itemCount);
            }
        }

        /// <summary>
        /// 设置显示游戏内物品Icon图片的委托
        /// 该委托接受挂载点GameObject和物品Id为参数
        /// </summary>
        /// <param name="showItemIconDelegate"></param>
        public void SetShowItemIconDelegate(Action<GameObject, uint> showItemIconDelegate)
        {
            _showItemIconDelegate = showItemIconDelegate;
        }

        public void ShowItemIcon(GameObject go, uint itemId)
        {
            if (_showItemIconDelegate != null)
            {
                _showItemIconDelegate(go, itemId);
            }
        }

        /// <summary>
        /// 设置显示道具tips委托
        /// 该委托的参数为tips挂载点GameObject和物品id
        /// </summary>
        /// <param name="showItemTipsDelegate"></param>
        public void SetShowItemTipsDelegate(Action<GameObject, uint> showItemTipsDelegate)
        {
            _showItemTipsDelegate = showItemTipsDelegate;
        }

        public void ShowItemTips(GameObject go, uint itemId)
        {
            if (_showItemTipsDelegate != null)
            {
                _showItemTipsDelegate(go, itemId);
            }
        }

        /// <summary>
        /// 设置游戏处理Pandora发往游戏的消息回调
        /// </summary>
        /// <param name="callback"></param>
        public void SetGameCallback(Action<Dictionary<string, string>> callback)
        {
            _gameCallback = callback;
        }

        public void SetJsonGameCallback(Action<string> callback)
        {
            _jsonGameCallback = callback;
        }

        private void ProgramAssetProgressCallback(Dictionary<string, string> dict)
        {
            CallGame(MiniJSON.Json.Serialize(dict));
        }

        public void CallGame(string jsonStr)
        {
            try
            {
                Dictionary<string, string> dict = DeserializeJson(jsonStr);
                _statusHelper.OnInternalEvent(dict);
                if (_jsonGameCallback != null)
                {
                    _jsonGameCallback(jsonStr);
                    return;
                }
                if (_gameCallback != null)
                {
                    _gameCallback(dict);
                }
            }
            catch (Exception e)
            {
                string error = "Lua发送消息,游戏回调发生异常, " + e.Message;
                Logger.LogError(error, PandoraSettings.SDKTag);
                Pandora.Instance.ReportError(error, ErrorCode.PANDORA_2_GAME_EXCEPTION);
                Pandora.Instance.Report(string.Format("Pandora2GameException:{0}|{1},{2},{3}", jsonStr, e.GetType().ToString(), e.Message, e.StackTrace), ErrorCode.PANDORA_2_GAME_EXCEPTION_DETAIL, ErrorCode.TNM2_TYPE_LITERALS);
            }
        }

        private Dictionary<string, string> DeserializeJson(string jsonStr)
        {
            Dictionary<string, System.Object> rawDict = MiniJSON.Json.Deserialize(jsonStr) as Dictionary<string, System.Object>;
            Dictionary<string, string> dict = new Dictionary<string, string>();
            var enumerator = rawDict.GetEnumerator();
            while (enumerator.MoveNext())
            {
                var kvp = enumerator.Current;
                dict[kvp.Key] = (kvp.Value == null) ? string.Empty : kvp.Value as string;
            }
            return dict;
        }

        /// <summary>
        /// 使用该接口，可以直接查询模块的准备情况，若资源准备失败的话，则发起重试流程
        /// </summary>
        /// <param name="groupName"></param>
        /// <returns></returns>
        public string GetStatus(string groupName, string functionSwitch = "")
        {
            string template = PandoraSettings.IsProductEnvironment ? PandoraStatusHelper.STATUS_PRODUCT_TEMPLATE : PandoraStatusHelper.STATUS_TEST_TEMPLATE;
            RemoteConfig remoteConfig = Pandora.Instance.GetRemoteConfig();
            if (remoteConfig != null && remoteConfig.assetInfoListDict.ContainsKey(groupName) == false)
            {
                return string.Format(template, "Source list empty");
            }
            if (remoteConfig != null && functionSwitch != "" && remoteConfig.GetFunctionSwitch(functionSwitch) == false)
            {
                return string.Format(template, "function switch closed");
            }
            string status = _statusHelper.GetStatus(groupName);
            if (status == PandoraStatusHelper.STATUS_CGI_FAILED || status == PandoraStatusHelper.STATUS_ASSET_FAILED)
            {
                LoadProgramAsset(groupName);
            }
            if (string.IsNullOrEmpty(status) == false)
            {
                return string.Format(template, status);
            }
            return string.Empty;
        }

        public void SetUserData(Dictionary<string, string> data)
        {
            data["sSdkVersion"] = GetSdkVersion();
            if (_userData.IsRoleEmpty())
            {
                _userData.Assign(data);
                _userData.CalculateSign();
                LoadRemoteConfig(_userData);
            }
            else
            {
                _userData.Assign(data);
            }
            Logger.LogInfo(_userData.ToString(), PandoraSettings.SDKTag);
        }

        public string CombinedSDKVersion()
        {
            return string.Format("{0}.{1}", PandoraSettings.SDKVersion, SdkBuildVersion);
        }

        private string GetSdkVersion()
        {
            if (PandoraSettings.Platform == RuntimePlatform.IPhonePlayer)
            {
                //return string.Format("{0}-IOS-{1}", GameCode, SdkVersion);
                return string.Format("{0}-IOS-{1}", GameCode, CombinedSDKVersion());
            }
            else if (PandoraSettings.Platform == RuntimePlatform.WindowsPlayer)
            {
                //return string.Format("{0}-PC-{1}", GameCode, SdkVersion);
                return string.Format("{0}-PC-{1}", GameCode, CombinedSDKVersion());
            }
            //return string.Format("{0}-Android-{1}", GameCode, SdkVersion);
            return string.Format("{0}-Android-{1}", GameCode, CombinedSDKVersion());
        }


        //对接阶段填入Broker域名
        private string GetBrokerHost()
        {
            if (PandoraSettings.IsProductEnvironment == true)
            {
                return _brokerHost;
            }
            else
            {
                return _brokerTestHost;
            }
        }

        //对接阶段填入Broker端口
        public int GetBrokerPort()
        {
            if (PandoraSettings.IsProductEnvironment == true)
            {
                return _brokerPort;
            }
            else
            {
                return _brokerTestPort;
            }
        }

        //对接阶段填入Broker备份IP1
        public string GetBrokerAlternateIp1()
        {
            if (PandoraSettings.IsProductEnvironment == true)
            {
                return _alternateIp1;
            }
            else
            {
                return _alternateTestIp1;
            }

        }
        //对接阶段填入Broker备份IP2
        public string GetBrokerAlternateIp2()
        {
            if (PandoraSettings.IsProductEnvironment == true)
            {
                return _alternateIp2;
            }
            else
            {
                return _alternateTestIp2;
            }
        }

        public string GetAtmHost()
        {
            if (PandoraSettings.IsProductEnvironment == true)
            {
                return _atmHost;
            }
            else
            {
                return _atmTestHost;
            }
        }

        public int GetAtmPort()
        {
            if (PandoraSettings.IsProductEnvironment == true)
            {
                return _atmPort;
            }
            else
            {
                return _atmTestPort;
            }
        }

        public string GetLogUploadUrl()
        {
            return PandoraSettings.IsProductEnvironment ? _logUploadUrl : _logUploadTestUrl;
        }

        public UserData GetUserData()
        {
            return _userData;
        }

        public void RefreshUserDataTokens()
        {
            if (_getAccountTokenDelegate != null)
            {
                _userData.Assign(_getAccountTokenDelegate());
            }
        }

        public void ReconnectRemoteConfigBroker()
        {
            if (_remoteConfigBrokerSocket != null)
            {
                _remoteConfigBrokerSocket.CleanBeforeClose();
                _remoteConfigBrokerSocket.Close();
                _remoteConfigBrokerSocket.Host = GetBrokerHost();
                _remoteConfigBrokerSocket.Port = GetBrokerPort();
            }
        }

        private void LoadRemoteConfig(UserData userData)
        {
            if (userData.IsValidate() == true)
            {
                _remoteConfig = null;
                _remoteConfigBrokerSocket.LoadRemoteConfig(userData, OnGetRemoteConfig);
            }
        }

        private void OnGetRemoteConfig(RemoteConfig remoteConfig)
        {
            if (remoteConfig == null)
            {
                return;
            }
            if (remoteConfig.IsError == true || remoteConfig.IsEmpty == true)
            {
                CallGame("{\"type\":\"pandoraError\",\"content\":\"cgiFailed\"}");
                return;
            }

            if (remoteConfig.IsEmpty == false && remoteConfig.IsError == false)
            {
                _remoteConfig = remoteConfig;
                SetLoggerConfig();
                LuaStateManager.Initialize();
                LoadFrameAsset();
            }
        }

        private void SetLoggerConfig()
        {
            if (PandoraSettings.LoginPlatform == PlatformType.InternalPlatform)
            {
                if (PandoraSettings.RequestCGIVersion == CGIVersion.Internal)
                {
                    var config = _remoteConfig as InternalPlatformRemoteConfig;
                    Logger.SetLogConfig(null, false, PandoraSettings.RequestCGIVersion.ToString(), config.isDebug, config.logLevel, false, Logger.ERROR);
                    PandoraSettings.IsDebug = _remoteConfig.isDebug;
                }

                if (PandoraSettings.RequestCGIVersion == CGIVersion.Uniform)
                {
                    //sdk层级的log由系统工具里的配置控制
                    Logger.Enable = _remoteConfig.isDebug;
                    Logger.LogLevel = _remoteConfig.logLevel;
                    UniformRemoteConfig config = _remoteConfig as UniformRemoteConfig;
                    Logger.SetLogConfig(config.LogConfigDict, config.GetFunctionSwitch("fullLogOutput"), PandoraSettings.RequestCGIVersion.ToString(), config.InterplatformConfig.isDebug, config.InterplatformConfig.logLevel, config.isDebug, config.logLevel);
                    PandoraSettings.IsDebug = config.isDebug || config.InterplatformConfig.isDebug;
                }
            }

            //开放平台暂未合入，先不处理
        }

        //加载lua基础设施frame
        private void LoadFrameAsset()
        {
            string key = string.Empty;
            List<RemoteConfig.AssetInfo> assetInfoList = null;
            if (_remoteConfig.assetInfoListDict.ContainsKey("frame") == true)
            {
                key = "frame";
                assetInfoList = _remoteConfig.assetInfoListDict["frame"];
            }
            if (_remoteConfig.assetInfoListDict.ContainsKey("Frame") == true)
            {
                key = "Frame";
                assetInfoList = _remoteConfig.assetInfoListDict["Frame"];
            }
            if (assetInfoList == null)
            {
                Logger.LogError("没有发现名为frame的资源组名，请正确配置管理端规则！ ", PandoraSettings.SDKTag);
            }
            else
            {
                AssetManager.LoadProgramAssetList(key, assetInfoList, OnFrameLoaded);
            }
        }

        private void OnFrameLoaded(string group, List<RemoteConfig.AssetInfo> fileInfoList)
        {
            LuaStateManager.DoLuaFileInFileInfoList(group, fileInfoList);
            ConnectBrokerSocket();
        }

        private void ConnectBrokerSocket()
        {
            _brokerSocket = GetBrokerSocket();
            if (PandoraSettings.LoginPlatform == PlatformType.InternalPlatform)
            {
                if (PandoraSettings.RequestCGIVersion == CGIVersion.Internal)
                {
                    _brokerSocket.AlternateIp1 = (_remoteConfig as InternalPlatformRemoteConfig).brokerAlternateIp1;
                    _brokerSocket.AlternateIp2 = (_remoteConfig as InternalPlatformRemoteConfig).brokerAlternateIp2;
                }

                if (PandoraSettings.RequestCGIVersion == CGIVersion.Uniform)
                {
                    _brokerSocket.AlternateIp1 = (_remoteConfig as UniformRemoteConfig).brokerAlternateIp1;
                    _brokerSocket.AlternateIp2 = (_remoteConfig as UniformRemoteConfig).brokerAlternateIp2;
                }
            }
            _brokerSocket.Connect(_remoteConfig.brokerHost, _remoteConfig.brokerPort, OnBrokerConnected);
        }

        private BrokerSocket GetBrokerSocket()
        {
            BrokerSocket[] brokerSockets = _gameObject.GetComponents<BrokerSocket>();
            for (int i = 0; i < brokerSockets.Length; i++)
            {
                if (brokerSockets[i].GetType().Name == "BrokerSocket")
                {
                    return brokerSockets[i];
                }
            }
            return _gameObject.AddComponent<BrokerSocket>();
        }

        private void OnBrokerConnected()
        {
            LoadProgramAsset();
        }

        internal void LoadProgramAsset()
        {
            var enumerator = _remoteConfig.assetInfoListDict.GetEnumerator();
            while (enumerator.MoveNext())
            {
                var kvp = enumerator.Current;
                if (kvp.Key.ToLower() != "frame")
                {
                    AssetManager.LoadProgramAssetList(kvp.Key, kvp.Value, OnLoaded);
                }
            }
        }

        public void LoadProgramAsset(string group)
        {
            Logger.Log("用户发起重连请求", PandoraSettings.SDKTag);
            if (_userData == null || _userData.IsRoleEmpty())
            {
                return;
            }

            if (_remoteConfig == null)
            {
                LoadRemoteConfig(_userData);
                ReportError("用户成功发起重连请求!", ErrorCode.START_RELOAD);
                return;
            }
            if (_remoteConfig != null && _remoteConfig.assetInfoListDict.ContainsKey(group) == true && LuaStateManager.IsGroupLuaExecuting(group) == false)
            {
                AssetManager.LoadProgramAssetList(group, _remoteConfig.assetInfoListDict[group], OnLoaded);
                ReportError("用户成功发起重连请求!", ErrorCode.START_RELOAD);
            }
        }

        private void OnLoaded(string group, List<RemoteConfig.AssetInfo> fileInfoList)
        {
            CallGame("{\"type\":\"assetLoadComplete\",\"name\":\"" + group + "\"}");
            LuaStateManager.DoLuaFileInFileInfoList(group, fileInfoList);
        }

        public bool IsProgramAssetReady(string group)
        {
            if (_remoteConfig == null || _remoteConfig.assetInfoListDict == null || _remoteConfig.assetInfoListDict.ContainsKey(group) == false)
            {
                return false;
            }

            return AssetManager.IsProgramAssetCached(_remoteConfig.assetInfoListDict[group]);
        }

        public RemoteConfig GetRemoteConfig()
        {
            return _remoteConfig;
        }

        internal BrokerSocket BrokerSocket
        {
            get
            {
                return _brokerSocket;
            }
        }

        internal AtmSocket AtmSocket
        {
            get
            {
                return _atmSocket;
            }
        }

        /// <summary>
        /// 设置Pandora模块使用的字体
        /// </summary>
        /// <param name="font"></param>
        public void SetFont(Font font)
        {
            if (font == null)
            {
                string errorMsg = "SetFont failed,font is null";
                ReportError(errorMsg);
                Logger.LogError(errorMsg, PandoraSettings.SDKTag);
                return;
            }

            if (_fontDict.ContainsKey(font.name) == false)
            {
                _fontDict.Add(font.name, font);
            }
            else
            {
                Logger.Log(string.Format("Repeat setfont {0}",font.name), PandoraSettings.SDKTag);
                _fontDict[font.name] = font;
            }
        }

        internal Font GetFont(string name)
        {
            if (_fontDict.ContainsKey(name))
            {
                Font cachedFont = _fontDict[name];
                if (cachedFont == null)
                {
                    string errorMsg = string.Format("CachedFont {0} is null",name);
                    ReportError(errorMsg);
                    Logger.LogError(errorMsg, PandoraSettings.SDKTag);
                    _fontDict.Remove(name);
                }
                else
                {
                    return cachedFont;
                }
            }

            Font font = Resources.Load<Font>(FontLoadDir + name);
            if (font != null)
            {
                Logger.Log(string.Format(" load font {0} from resources directory successfully",font.name), PandoraSettings.SDKTag);
                _fontDict.Add(name, font);
                return font;
            }

            string msg = string.Format("get font {0} failed",name);
            ReportError(msg);
            Logger.LogError(msg, PandoraSettings.SDKTag);
            return null;
        }


        /// <summary>
        /// 游戏往Pandora发送消息
        /// </summary>
        /// <param name="commandDict"></param>
        public void Do(Dictionary<string, string> commandDict)
        {
            CSharpInterface.GameCallLua(MiniJSON.Json.Serialize(commandDict));
        }

        public void DoJson(string jsonStr)
        {
            CSharpInterface.GameCallLua(jsonStr);
        }

        public bool PauseDownloading
        {
            get
            {
                return PandoraSettings.PauseDownloading;
            }
            set
            {
                PandoraSettings.PauseDownloading = value;
            }
        }

        public bool PauseSocketSending
        {
            get
            {
                return PandoraSettings.PauseSocketSending;
            }
            set
            {
                PandoraSettings.PauseSocketSending = value;
            }
        }

        public void ReportError(string error, int id = 0)
        {
            Report(error, id, 0);
        }

        public void Report(string content, int id, int type)
        {
            if (_atmSocket != null)
            {
                _atmSocket.Report(content, id, type);
            }
        }

        // 是否要将Log保存在本地
        public bool IsDebug
        {
            get
            {
                return PandoraSettings.IsDebug;
            }
        }

        public string GameCode
        {
            get
            {
                if (_forceGameCodeToUpper)
                {
                    return _gameCode.ToUpper();
                }
                return _gameCode.ToLower();
            }
            set { _gameCode = value; }
        }

        public object CallLuaFunction(string functionName, params System.Object[] args)
        {
            if (LuaStateManager.IsInitialized == true)
            {
                return LuaStateManager.CallLuaFunction(functionName, args);
            }
            return null;
        }

        public void GC()
        {
            if (_isInitialized == true)
            {
                object o = CallLuaFunction("Common.LuaGC");
                LuaStateManager.Release(o);
                AssetManager.DeleteZeroReferenceAsset(true);
            }
        }

        private void SendLogoutMessage()
        {
            Dictionary<string, string> dict = new Dictionary<string, string>();
            dict.Add("type", "logout");
            Do(dict);
        }

        public void Logout()
        {
            try
            {
                if (_isInitialized == true)
                {
                    SendLogoutMessage();
                    if (_atmSocket != null)
                    {
                        _atmSocket.CleanBeforeClose();
                        _atmSocket.Close();
                    }
                    if (_brokerSocket != null)
                    {
                        _brokerSocket.CleanBeforeClose();
                        _brokerSocket.Close();
                    }
                    if (_remoteConfigBrokerSocket != null)
                    {
                        _remoteConfigBrokerSocket.CleanBeforeClose();
                        _remoteConfigBrokerSocket.Close();
                    }
                    _statusHelper.Reset();
                    PanelManager.DestroyAll();
                    LuaStateManager.Reset();
                    AssetManager.Clear();
                    Logger.Dispose();
                    _userData.Clean();
                    _remoteConfig = null;
                }
            }
            catch (Exception e)
            {
                Logger.LogError("Pandora Logout发生异常: " + e.Message + "|" + e.StackTrace, PandoraSettings.SDKTag);
            }
        }

        #region 设置SDK层的委托
        public void InitDelegate()
        {
            DelegateAggregator.SetReportDelegate(Report);
            DelegateAggregator.SetGetUserDataDelegate(GetUserData);
            DelegateAggregator.SetGetRemoteConfigDelegate(GetRemoteConfig);
            DelegateAggregator.SetGetGameObjectDelegate(GetGameObject);
            DelegateAggregator.SetGetPanelRootDelegate(GetPanelRoot);
            DelegateAggregator.SetApplyPanelTextureDelegate(ApplyPanelTexture);
            DelegateAggregator.SetGetUIMaterialListDelegate(GetUIMaterialList);
            DelegateAggregator.SetGetGameObjectTextureSetDelegate(GetGameObjectTextureSet);
            DelegateAggregator.SetExecuteLuaCallbackDelegate(CSharpInterface.ExecuteLuaCallback);
            DelegateAggregator.SetGetLogUploadUrlDelegate(GetLogUploadUrl);
        }

        private List<Material> GetUIMaterialList(GameObject go)
        {
            List<Material> result = new List<Material>();
#if USING_NGUI
            UISprite[] sprites = go.GetComponentsInChildren<UISprite>(true);
            for (int i = 0; i < sprites.Length; i++)
            {
                Material material = sprites[i].material;
                if (material != null)
                {
                    result.Add(material);
                }
            }

            UITexture[] textures = go.GetComponentsInChildren<UITexture>(true);
            for (int i = 0; i < textures.Length; i++)
            {
                Material material = textures[i].material;
                if (material != null)
                {
                    result.Add(material);
                }
            }
#endif
#if USING_UGUI
            UnityEngine.UI.Image[] images = go.GetComponentsInChildren<UnityEngine.UI.Image>(true);
            for (int i = 0; i < images.Length; i++)
            {
                Material material = images[i].material;
                if (material != null)
                {
                    result.Add(material);
                }
            }

            UnityEngine.UI.RawImage[] rawImages = go.GetComponentsInChildren<UnityEngine.UI.RawImage>(true);
            for (int i = 0; i < rawImages.Length; i++)
            {
                Material material = rawImages[i].material;
                if (material != null)
                {
                    result.Add(material);
                }
            }
#endif
            return result;
        }

        private bool ApplyPanelTexture(GameObject go, Texture2D texture)
        {
            if (go == null || texture == null)
            {
                return false;
            }
            TextureHelper helper = go.GetComponent<TextureHelper>();
            if (helper != null)
            {
                helper.Show(texture);
                return true;
            }
#if USING_NGUI
            UITexture uiTexture = go.GetComponent<UITexture>();
            if (uiTexture != null)
            {
                uiTexture.mainTexture = texture;
                return true;
            }
#endif
#if USING_UGUI
            UnityEngine.UI.RawImage rawImage = go.GetComponent<UnityEngine.UI.RawImage>();
            if (rawImage != null)
            {
                rawImage.texture = texture;
                return true;
            }
            UnityEngine.UI.Image image = go.GetComponent<UnityEngine.UI.Image>();
            if (image != null)
            {
                Sprite sprite = Sprite.Create(texture, new Rect(0f, 0f, texture.width, texture.height), new Vector2(0.5f, 0.5f));
                image.sprite = sprite;
                return true;
            }
#endif
            return false;
        }

        private HashSet<Texture> GetGameObjectTextureSet(GameObject go)
        {
            HashSet<Texture> result = new HashSet<Texture>();
#if USING_UGUI
            UnityEngine.UI.Image[] images = go.GetComponentsInChildren<UnityEngine.UI.Image>(true);
            for (int i = 0; i < images.Length; i++)
            {
                Sprite sprite = images[i].sprite;
                if (sprite != null)
                {
                    result.Add(sprite.texture);
                }
            }
            UnityEngine.UI.RawImage[] rawImages = go.GetComponentsInChildren<UnityEngine.UI.RawImage>(true);
            for (int i = 0; i < rawImages.Length; i++)
            {
                UnityEngine.UI.RawImage rawImage = rawImages[i];
                if (rawImage.texture != null)
                {
                    result.Add(rawImage.texture);
                }
            }
#endif
#if USING_NGUI
            UISprite[] sprites = go.GetComponentsInChildren<UISprite>(true);
            for (int i = 0; i < sprites.Length; i++)
            {
                UIAtlas atlas = sprites[i].atlas;
                if (atlas != null)
                {
                    result.Add(atlas.texture);
                }
            }
            UITexture[] textures = go.GetComponentsInChildren<UITexture>(true);
            for (int i = 0; i < textures.Length; i++)
            {
                UITexture texture = textures[i];
                if (texture.mainTexture != null)
                {
                    result.Add(texture.mainTexture);
                }
            }
#endif
            return result;
        }
        #endregion

        #region For AssetPoolInspector
        public static Dictionary<string, int> GetAssetReferenceCountDict()
        {
            return AssetPool.GetAssetReferenceCountDict();
        }

        public static int GetLuaUsedMemory()
        {
            object o = Pandora.Instance.CallLuaFunction("Common.GetLuaUsedMemory");
            if (o == null)
            {
                return 0;
            }
            int result = 0;
            int.TryParse(o.ToString(), out result);
            LuaStateManager.Release(o);
            return result;
        }

        public static Dictionary<string, HashSet<Texture>> GetPrefabTextureSetDict()
        {
            return AssetPool.GetPrefabTextureSetDict();
        }
        #endregion

    }
}
