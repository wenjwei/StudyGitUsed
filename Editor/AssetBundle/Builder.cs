using System;
using UnityEngine;
using UnityEditor;
using System.IO;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using UnityEngine.UI;
using Object = UnityEngine.Object;

namespace com.tencent.pandora.tools
{
    /// <summary>
    /// 资源打包主要策略：
    /// 1.Prefab文件和其依赖的资源打成一个包，若同一个项目中Pandora面板数量较多且存在共享资源时，则可以将图集资源提取出来，做成依赖打包
    /// 2.Actions/Resources目录下一个目录的Lua文件打成一个包
    /// </summary>
    internal class Builder
    {
        private const string COLOR_SHADER = "Unlit/Transparent Colored";
        private const string ALPHA_SHADER = "Unlit/Seperate Alpha Colored";

        private const string COPY_TOKEN = "_copy";
        /// <summary>
        /// 一个PathList中的资源打成一个Bundle
        /// </summary>
        private static List<List<string>> _assetPathListList;
        /// <summary>
        /// 生成的Bundle路径列表
        /// </summary>
        private static List<string> _bundlePathList;
        private static List<string> _copyPrefabList;

        private static Dictionary<BuildTarget, string> BUILD_TARGET_NAME_DICT = new Dictionary<BuildTarget, string>()
        {
            { BuildTarget.StandaloneWindows, "pc" },
            { BuildTarget.StandaloneWindows64, "pc" },
            { BuildTarget.Android, "android" },
            { BuildTarget.StandaloneOSX, "ios" },
#if UNITY_4_7
            { BuildTarget.iPhone, "ios" },
#elif UNITY_5 || UNITY_2017_1_OR_NEWER
            { BuildTarget.iOS, "ios" }
#endif
        };

        private static void Initialize()
        {
            _assetPathListList = new List<List<string>>();
            _bundlePathList = new List<string>();
            _copyPrefabList = new List<string>();
        }

        public static void Build(List<string> activityList, BuildTarget target, bool showReport = true)
        {
            LuaProcessor.PreProcessLuaFile();
			Initialize();
            CreateStreamingAssetFolder();
            BuildActivityPrefabList(activityList, target);
            ComplieActivityLuaList(activityList);
            BuildActivityLuaList(activityList, target, BuilderSetting.LUA_32_PATH_TEMPLATE);
            BuildActivityLuaList(activityList, target, BuilderSetting.LUA_64_PATH_TEMPLATE);
            ExecuteBuild(target);
            DeleteCompiledLuaList(activityList);
            DeleteCopyPrefabList();
			LuaProcessor.PostProcessLuaFile();
            AssetDatabase.Refresh();
            //CopyBundleToGame("G:/Speedm_2017/NssUnityProj/Assets/StreamingAssets/Pandora");
            if(showReport == true)
            {
                ShowReport();
            }
        }

        private static void CreateStreamingAssetFolder()
        {
            if (!Directory.Exists(BuilderSetting.STREAM_ASSET_PATH))
            {
                Directory.CreateDirectory(BuilderSetting.STREAM_ASSET_PATH);
            }
        }

        /// <summary>
        /// 打包某个Activity的Prefab文件列表
        /// 其中Prefab文件和图集资源根据依赖关系打在一起，若将来存在公共资源的情况时需要优化
        /// 文本的字体资源会剥离出来，不打包
        /// </summary>
        /// <param name="activityList"></param>
        /// <param name="target"></param>
        private static void BuildActivityPrefabList(List<string> activityList, BuildTarget target)
        {
            foreach(string s in activityList)
            {
                if(ActivityManager.IsActivityBuildPrefab(s) == true)
                {
                    string prefabFolder = string.Format(BuilderSetting.PREFAB_PATH_TEMPLATE, s);
                    List<string> prefabPathList = GetPrefabPathList(prefabFolder);
                    foreach (string path in prefabPathList)
                    {
                        BuildPrefab(path, target);
                    }
                }
            }
        }

        private static List<string> GetPrefabPathList(string prefabFolder)
        {
            List<string> result = new List<string>();
            string[] guids = AssetDatabase.FindAssets("t:GameObject", new string[] { prefabFolder });
            foreach (string s in guids)
            {
                string path = AssetDatabase.GUIDToAssetPath(s);
                if(path.Contains("_copy.prefab") == false)
                {
                    result.Add(path);
                }
            }
            return result;
        }

        private static void BuildPrefab(string path, BuildTarget target)
        {
            string copyPath = CreatePolishedPrefab(path, target);
            _copyPrefabList.Add(copyPath);
            _assetPathListList.Add(new List<string>() { copyPath });
        }

        /// <summary>
        /// 1.剥离font资源
        /// 2.替换目标平台资源
        /// </summary>
        /// <param name="path"></param>
        private static string CreatePolishedPrefab(string path, BuildTarget target)
        {
            string copyPath = path.Replace(".prefab", COPY_TOKEN + ".prefab");
            AssetDatabase.DeleteAsset(copyPath);
            AssetDatabase.CopyAsset(path, copyPath);
            AssetDatabase.ImportAsset(copyPath, ImportAssetOptions.ForceUpdate);
            GameObject prefab = AssetDatabase.LoadAssetAtPath(copyPath, typeof(GameObject)) as GameObject;

            GameObject go = PrefabUtility.InstantiatePrefab(prefab) as GameObject;
            RefreshVersion(go);
            //NGUI 版本 UILabel使用NGUI Font
            UILabel[] labels = go.GetComponentsInChildren<UILabel>(true);
            foreach(UILabel l in labels)
            {
                if(l.trueTypeFont != null)
                {
                    TextPartner partner = l.gameObject.AddComponent<TextPartner>();
                    partner.fontName = l.ambigiousFont.name;
                    l.trueTypeFont = null;
                    l.bitmapFont = null;
                    l.ambigiousFont = null;
                }
            }
            UIPopupList[] popups = go.GetComponentsInChildren<UIPopupList>(true);
            foreach (UIPopupList l in popups)
            {
                if (l.trueTypeFont != null)
                {
                    TextPartner partner = l.gameObject.AddComponent<TextPartner>();
                    partner.fontName = l.ambigiousFont.name;
                    l.trueTypeFont = null;
                    l.bitmapFont = null;
                    l.ambigiousFont = null;
                }
            }
            //1.检查是否有冗余资源
            //2.去除对游戏公共资源的依赖，记录依赖信息
            UISprite[] sprites = go.GetComponentsInChildren<UISprite>(true);
            HashSet<string> atlasNameSet = new HashSet<string>();
            foreach(UISprite s in sprites)
            {
                if(s.atlas != null && atlasNameSet.Contains(s.atlas.name) == false)
                {
                    atlasNameSet.Add(s.atlas.name);
                    CheckNGUIAtlas(s.atlas);
                }
                AddUISpritePartner(s);
                AddUIAtlasPartner(s);
            }

            //含有MaterialPartner的组件，记录Material信息，并将material置空
            MaterialPartner[] partners = go.GetComponentsInChildren<MaterialPartner>(true);
            foreach(MaterialPartner p in partners)
            {
                Renderer renderer = p.gameObject.GetComponent<Renderer>();
                if(renderer != null && renderer.sharedMaterials != null && renderer.sharedMaterials.Length > 0)
                {
                    string[] materialNames = new string[renderer.sharedMaterials.Length];
                    for(int i = 0; i < renderer.sharedMaterials.Length; i++)
                    {
                        if(renderer.sharedMaterials[i] != null)
                        {
                            materialNames[i] = renderer.sharedMaterials[i].name;
                        }
                    }
                    p.materialNames = materialNames;
                    renderer.sharedMaterials = new Material[0];
                }
            }

            /*UGUI 版本*/
            Text[] texts = go.GetComponentsInChildren<Text>(true);
            foreach(Text t in texts)
            {
                if(t.font != null)
                {
                    TextPartner partner = t.gameObject.AddComponent<TextPartner>();
                    partner.fontName = t.font.name; //TODO:检查fontName和资源名是否一致
                    t.font = null;
                }
            }

            /*UGUI 版本
            Image[] images = go.GetComponentsInChildren<Image>(true);
            foreach(Image i in images)
            {
                if(i.sprite != null)
                {
                    i.sprite = GetCurrentPlatformSprite(atlasName, i.sprite, target);
                }
                if(i.material != null)
                {
                    i.material = GetCurrentPlatformMaterial(atlasName, i.material, target);
                }
            }
            */
            AddPanelOnDestroyHook(go);
            PrefabUtility.CreatePrefab(copyPath, go, ReplacePrefabOptions.ConnectToPrefab);
            Object.DestroyImmediate(go, true);
            return copyPath;
        }

        private static void CheckNGUIAtlas(UIAtlas atlas)
        {
            HashSet<string> nameSet = new HashSet<string>();
            foreach(UISpriteData s in atlas.spriteList)
            {
                nameSet.Add(s.name);
            }
            foreach(UISpriteData s in atlas.spriteList)
            {
                if(s.name.Contains("_scale9") == true)
                {
                    if(nameSet.Contains(s.name.Replace("_scale9", "")) == true)
                    {
                        EditorUtility.DisplayDialog("警告", "图集中存9宫图片的原始图片:  " + s.name, "马上处理~");
                    }
                }
                if(s.name.Contains("_clamp") == true)
                {
                    if (nameSet.Contains(s.name.Replace("_clamp", "")) == true)
                    {
                        EditorUtility.DisplayDialog("警告", "图集中存补边图片的原始图片:  " + s.name, "马上处理~");
                    }
                }
            }
        }

        /// <summary>
        /// 使用游戏的公共资源，以减少模块图集资源量
        /// </summary>
        /// <param name="sprite"></param>
        private static void AddUISpritePartner(UISprite sprite)
        {
            if(sprite.atlas != null)
            {
                string atlasPath = AssetDatabase.GetAssetPath(sprite.atlas.gameObject);
                if(atlasPath.Contains("Actions/Resources/") == false)
                {
                    UISpritePartner partner = sprite.gameObject.AddComponent<UISpritePartner>();
                    partner.atlasName = sprite.atlas.name;
                    partner.spriteName = sprite.spriteName;
                    sprite.atlas = null;
                    sprite.spriteName = string.Empty;
                }
            }
        }

        private static void AddUIAtlasPartner(UISprite sprite)
        {
            if(sprite.atlas != null)
            {
                string atlasPath = AssetDatabase.GetAssetPath(sprite.atlas.gameObject);
                if (atlasPath.Contains("Actions/Resources/") == true)
                {
                    UIAtlasPartner partner = sprite.gameObject.AddComponent<UIAtlasPartner>();
                    partner.atlas = sprite.atlas;
                }
            }
        }

        private static void RefreshVersion(GameObject go)
        {
            Transform trans = go.transform.Find("Label_version");
            if(trans != null)
            {
                UILabel label = trans.GetComponent<UILabel>();
                label.text = "Ver: " + DateTime.Now.ToString("MM-dd-HH-mm");
            }
        }

        private static void AddPanelOnDestroyHook(GameObject go)
        {
            go.AddComponent<com.tencent.pandora.PanelOnDestroyHook>();
        }

        /// <summary>
        /// 获取当前平台的资源
        /// </summary>
        /// <param name="atlasName"></param>
        /// <param name="sprite"></param>
        /// <returns></returns>
        private static Sprite GetCurrentPlatformSprite(string atlasName, Sprite sprite, BuildTarget target)
        {
            string androidToken = atlasName + "_android";
            string iosToken = atlasName + "_ios";
            string platfrom = GetPlatformName(target);
            string assetPath = AssetDatabase.GetAssetPath(sprite);
            UnityEngine.Object[] objs = null;
            if(platfrom == "android" && assetPath.Contains(iosToken))
            {
                string andoridAtlasPath = assetPath.Replace(iosToken, androidToken);
                objs = AssetDatabase.LoadAllAssetsAtPath(andoridAtlasPath);
            }
            else if(platfrom == "ios" && assetPath.Contains(androidToken))
            {
                string iosAtlasPath = assetPath.Replace(androidToken, iosToken);
                objs = AssetDatabase.LoadAllAssetsAtPath(iosAtlasPath);
            }
            if(objs != null && objs.Length > 0)
            {
                foreach(UnityEngine.Object o in objs)
                {
                    if (o.name == sprite.name && o.GetType() == typeof(Sprite))
                    {
                        return o as Sprite;
                    }
                }
            }
            return sprite;
        }

        private static Material GetCurrentPlatformMaterial(string atlasName, Material material, BuildTarget target)
        {
            string androidToken = atlasName + "_android";
            string iosToken = atlasName + "_ios";
            string platfrom = GetPlatformName(target);
            string assetPath = AssetDatabase.GetAssetPath(material);
            string path = string.Empty;
            if (platfrom == "android" && assetPath.Contains(iosToken))
            {
                path = assetPath.Replace(iosToken, androidToken);
            }
            else if (platfrom == "ios" && assetPath.Contains(androidToken))
            {
                path = assetPath.Replace(androidToken, iosToken);
            }
            if(string.IsNullOrEmpty(path) == false)
            {
                Material platformMaterial = AssetDatabase.LoadAssetAtPath(path, typeof(Material)) as Material;
                if(platformMaterial != null)
                {
                    return platformMaterial;
                }
            }
            return material;
        }

        private static string GetPrefabBundleName(string path, BuildTarget target)
        {
            string result = string.Empty;
            Object obj = AssetDatabase.LoadAssetAtPath(path, typeof(GameObject));
            result = GetPlatformName(target) + "_" + obj.name.Replace(COPY_TOKEN, "") + ".assetbundle";
            return result;
        }

        private static void DeleteCompiledLuaList(List<string> activityList)
        {
            foreach (string s in activityList)
            {
                string lua32Folder = string.Format(BuilderSetting.LUA_32_PATH_TEMPLATE, s);
                string lua64Folder = string.Format(BuilderSetting.LUA_64_PATH_TEMPLATE, s);
                AssetDatabase.DeleteAsset(lua32Folder);
                AssetDatabase.DeleteAsset(lua64Folder);
            }
        }

        private static void ComplieActivityLuaList(List<string> activityList)
        {
            LuaCompilerWrapper.Error = string.Empty;
            foreach (string s in activityList)
            {
                string luaFolder = string.Format(BuilderSetting.LUA_PATH_TEMPLATE, s);
                List<string> luaPathList = GetLuaPathList(luaFolder);
                foreach(string p in luaPathList)
                {
                    string path = string.Concat(Application.dataPath.Replace("/Assets", "/"), p);
                    LuaCompilerWrapper.Compile(path);
                }
            }
            if(string.IsNullOrEmpty(LuaCompilerWrapper.Error) == false)
            {
                EditorUtility.DisplayDialog("Lua编译发生错误", LuaCompilerWrapper.Error, "改改改~");
            }
            AssetDatabase.Refresh();
            
        }

        /// <summary>
        /// 每一个Activity的所有Lua文件合并打成一个包
        /// </summary>
        /// <param name="activityList"></param>
        /// <param name="target"></param>
        private static void BuildActivityLuaList(List<string> activityList, BuildTarget target, string luaPathTemplate)
        {
            foreach(string s in activityList)
            {
                string luaFolder = string.Format(luaPathTemplate, s);
                List<string> luaPathList = GetLuaPathList(luaFolder);
                if(luaPathList.Count > 0)
                {
                    _assetPathListList.Add(luaPathList);
                }
            }
        }

        private static List<string> GetLuaPathList(string luaFolder)
        {
            List<string> result = Directory.GetFiles(luaFolder, "*.lua.bytes", SearchOption.AllDirectories).Where<string>((s) => { return s.Contains(".meta") == false; }).ToList<string>();
            return result;
        }

        private static string GetLuaBundleName(string luaAssetPath, BuildTarget target)
        {
            string dicPath = Path.GetDirectoryName(luaAssetPath);
            string architectureName = Path.GetFileName(dicPath);
            string dicPath2 = Path.GetDirectoryName(dicPath);
            string activityName = Path.GetFileName(dicPath2);
            string postfix = "_lua32.assetbundle";
            if(architectureName == "Lua64")
            {
                postfix = "_lua64.assetbundle";
            }
            string result = string.Empty;

            result = GetPlatformName(target) + "_" + activityName + postfix;
            return result;
        }

        private static void ExecuteBuild(BuildTarget target)
        {
#if UNITY_4_7
            ExecuteBuild_4_7(target);

#elif UNITY_5 || UNITY_2017_1_OR_NEWER
            ExecuteBuild_5_3(target);
#endif
        }

#if UNITY_4_7
        private static void ExecuteBuild_4_7(BuildTarget target)
        {
            for (int i = 0; i < _assetPathListList.Count; i++)
            {
                List<string> assetPathList = _assetPathListList[i];

                if (assetPathList[0].ToLower().Contains(".prefab") == true)
                {
                    BuildAssetList(assetPathList, target, typeof(GameObject), GetPrefabBundleName);
                }
                else if(assetPathList[0].ToLower().Contains(".lua") == true)
                {
                    BuildAssetList(assetPathList, target, typeof(TextAsset), GetLuaBundleName);
                }
            }
        }

        private static void BuildAssetList(List<string> assetPathList, BuildTarget target, Type assetType, Func<string, BuildTarget, string> getBundleName)
        {
            UnityEngine.Object[] assets = new UnityEngine.Object[assetPathList.Count];
            for(int i = 0; i< assetPathList.Count; i++)
            {
                assets[i] = AssetDatabase.LoadAssetAtPath(assetPathList[i], assetType);
            }
            
            string bundleName = getBundleName(assetPathList[0], target).ToLower();
            string bundlePath = BuilderSetting.STREAM_ASSET_PATH + "/" + bundleName;
            BuildPipeline.BuildAssetBundle(assets[0], assets, bundlePath, GetBuildOptions(), target);
            _bundlePathList.Add(bundlePath);
        }

        private static BuildAssetBundleOptions GetBuildOptions()
        {
            return BuildAssetBundleOptions.CollectDependencies | BuildAssetBundleOptions.DeterministicAssetBundle | BuildAssetBundleOptions.CompleteAssets;
        }
#endif

#if UNITY_5 || UNITY_2017_1_OR_NEWER
        private static void ExecuteBuild_5_3(BuildTarget target)
        {
            List<AssetBundleBuild> buildList = new List<AssetBundleBuild>();
            for(int i = 0; i < _assetPathListList.Count; i++)
            {
                List<string> assetPathList = _assetPathListList[i];
                if (assetPathList[0].ToLower().Contains(".prefab") == true)
                {
                    buildList.Add(BuildAssetList(assetPathList, target, GetPrefabBundleName));
                }
                else if(assetPathList[0].ToLower().Contains(".lua") == true)
                {
                    buildList.Add(BuildAssetList(assetPathList, target, GetLuaBundleName));
                }
            }
            BuildPipeline.BuildAssetBundles(BuilderSetting.STREAM_ASSET_PATH, buildList.ToArray(), BuildAssetBundleOptions.DeterministicAssetBundle, target);
        }

        private static AssetBundleBuild BuildAssetList(List<string> assetPathList, BuildTarget target, Func<string, BuildTarget, string> getBundleName)
        {
            AssetBundleBuild build = new AssetBundleBuild();
            build.assetBundleName = getBundleName(assetPathList[0], target).ToLower();
            build.assetNames = assetPathList.ToArray();
            _bundlePathList.Add(BuilderSetting.STREAM_ASSET_PATH + "/" + build.assetBundleName);
            return build;
        }
#endif

        private static void DeleteCopyPrefabList()
        {
            foreach(string s in _copyPrefabList)
            {
                AssetDatabase.DeleteAsset(s);
            }
        }

        private static void ShowReport()
        {
            if(_bundlePathList.Count == 0)
            {
                EditorUtility.DisplayDialog("打包结果： ", "需要打包的文件列表为空", "朕知道了~");
                return;
            }
            foreach(string path in _bundlePathList)
            {
                //AssetDatabase.ImportAsset(path);
                StringBuilder sb = new StringBuilder();
                sb.Append("生成文件： ");
                sb.Append(path);
                sb.Append(" 体积： ");
                sb.Append(GetFileSize(path));
                sb.Append("kb");
                Debug.Log("<color=#0000ff>" + sb.ToString() + "</color>");
            }
        }

        //打包后将资源复制到游戏工程StreamingAssets目录下
        private static void CopyBundleToGame(string targetFolderPath)
        {
            try
            {
                if(Directory.Exists(targetFolderPath) == false)
                {
                    Directory.CreateDirectory(targetFolderPath);
                }
                foreach (string path in _bundlePathList)
                {
                    string targetPath = Path.Combine(targetFolderPath, Path.GetFileName(path));
                    File.Copy(path, targetPath, true);
                }
            }
            catch
            {
                Debug.LogError("复制bundle到 " + targetFolderPath + " 失败~");
            }
        }

        private static int GetFileSize(string path)
        {
            FileInfo info = new FileInfo(path);
            if(info.Exists == false)
            {
                Debug.LogError("未找到文件: " + path);
            }
            return Mathf.CeilToInt(info.Length / 1024.0f);
        }

        public static string GetPlatformName(BuildTarget target)
        {
            string result = string.Empty;
            if(BUILD_TARGET_NAME_DICT.ContainsKey(target))
            {
                result = BUILD_TARGET_NAME_DICT[target];
            }
            else
            {
                throw new Exception("发现未预定义平台描述信息~，请先添加相关信息。");
            }
            return result;
        }

    }

}
