#define USING_NGUI
//#define USING_UGUI
using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using UnityEngine.UI;

namespace com.tencent.pandora
{
    public class PandoraToolBoxConfig : EditorWindow
    {
        #region 参数
        private Font font;

        //PandoraToolBoxSettings.cs 的配置
        private Dictionary<string, string> configDict = new Dictionary<string, string>()
    {
        {"PandoraToolBoxSettings.actStyle",""},
        {"PandoraToolBoxSettings.channelId",""},
        {"PandoraToolBoxSettings.actionId",""},
        {"PandoraToolBoxSettings.infoId",""},
        {"PandoraToolBoxSettings.infoId_test",""},
       
    };

        private Dictionary<string, string> configDescriptionDict = new Dictionary<string, string>()
    {
        {"PandoraToolBoxSettings.actStyle","活动类型（actStyle）"},
        {"PandoraToolBoxSettings.channelId","所属频道（channelId）"},
        {"PandoraToolBoxSettings.actionId","关联的道聚城活动id"},
        {"PandoraToolBoxSettings.infoId","正式环境管理端活动id"},
        {"PandoraToolBoxSettings.infoId_test","测试环境管理端活动id"},        
    };

        private Vector2 scrollPosition = Vector2.zero;
        private string builderFilePath = "Editor/Pandora/Editor/AssetBundle/Builder.cs";
        private string hasInsertCodeIntoBuilderCookiePath = "Editor/Pandora/Editor/ToolBoxConfig/hasInsertCodeIntoBuilderCookie.txt";
        private string hasSubstitutePrefabFontCookiePath = "Editor/Pandora/Editor/ToolBoxConfig/hasSubstitutePrefabFontCookie.txt";
        private string UGUIPrefabPath = "Assets/Actions/Resources/PandoraToolBox/Prefabs/PandoraToolBox_UGUI.prefab";
        private string NGUIPrefabPath = "Assets/Actions/Resources/PandoraToolBox/Prefabs/PandoraToolBox_NGUI.prefab";
        private string prefabPath;
        private string settingFilePath = "Actions/Resources/PandoraToolBox/Lua/PandoraToolBoxSettings.lua.bytes";

        #endregion

        [MenuItem("Assets/PandoraToolBoxConfig")]
        private static void Init()
        {
            PandoraToolBoxConfig configWindow = (PandoraToolBoxConfig)EditorWindow.GetWindow(typeof(PandoraToolBoxConfig), false, "ConfigWindow");
            configWindow.position = new Rect(200, 200, 360, 270);
            configWindow.Show(true);
        }

        [MenuItem("Assets/PandoraToolBoxConfig", true)]
        private static bool MenuValidation()
        {
            string activeOjectPath = AssetDatabase.GetAssetPath(Selection.activeObject);
            if (activeOjectPath.EndsWith("GUI.prefab"))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        private void OnGUI()
        {
            EditorGUILayout.BeginVertical();
            GUILayout.Space(10);
            DrawFontArea();
            DrawSettingsConfig();
            DrawButton();
            EditorGUILayout.EndVertical();
        }

        private void DrawFontArea()
        {
            DrawTitle("面板使用的字体");
#if UNITY_4
            font = EditorGUILayout.ObjectField("字体：", font, typeof(Font)) as Font;
#else
            font = EditorGUILayout.ObjectField("字体：", font, typeof(Font), true) as Font;
#endif
        }

        private void DrawTitle(string title)
        {
            GUIStyle titleStyle = new GUIStyle();
            titleStyle.fontSize = 15;
            titleStyle.normal.textColor = Color.green;
            EditorGUILayout.LabelField(title + "：", titleStyle);
            DrawSpaceLine(2);
        }

        private void DrawSpaceLine(int rows)
        {
            for (int i = 0; i < rows; i++)
            {
                EditorGUILayout.Space();
            }
        }

        void DrawSettingsConfig()
        {
            DrawConfigList("支付测试活动的参数", ref scrollPosition, ref configDict, ref configDescriptionDict);
        }

        private void DrawConfigList(string title, ref Vector2 scrollPosition, ref Dictionary<string, string> configDict, ref Dictionary<string, string> configDescriptionDict)
        {
            DrawTitle(title);
            scrollPosition = EditorGUILayout.BeginScrollView(scrollPosition, GUILayout.Height(150));
            List<string> configList = new List<string>(configDict.Keys);
            string key = "";
            for (int i = 0; i < configList.Count; i++)
            {
                key = configList[i];
                configDict[key] = EditorGUILayout.TextField(configDescriptionDict[key] + ":", configDict[key]).Trim();
            }
            EditorGUILayout.EndScrollView();
        }

        private void DrawButton()
        {
            if (GUILayout.Button("开始调整", GUILayout.Height(50)))
            {
                InsertCodeIntoBuilerFile();
                SubstitutePrefabFont();
                SubstituteDJCParams();
            }
        }

        private void InsertCodeIntoBuilerFile()
        {
            if (HasConfiged(hasInsertCodeIntoBuilderCookiePath))
            {
                DisplayWarningDialog("Builder.cs 中已插入相关代码");
                return;
            }
            string absoluteBuilderFilePath = Path.Combine(Application.dataPath, builderFilePath);
            string headPattern = @"Initialize\(\);";
            string headTarget = "LuaProcessor.PreProcessLuaFile();\r\n\t\t\tInitialize();";
            string footPattern = @"DeleteCopyPrefabList\(\);";
            string footTarget = "DeleteCopyPrefabList();\r\n\t\t\tLuaProcessor.PostProcessLuaFile();";
            if (!File.Exists(absoluteBuilderFilePath))
            {
                string tips = "不存在，无法自动向Builder.cs中插入代码。请手动在Builder函数开头插入LuaProcessor.PreProcessLuaFile();" +
                              "在Build函数末尾插入LuaProcessor.PostProcessLuaFile();";
                DisplayWarningDialog(absoluteBuilderFilePath + tips);
                return;
            }

            string builderFile = File.ReadAllText(absoluteBuilderFilePath);
            builderFile = Regex.Replace(builderFile, headPattern, headTarget);
            builderFile = Regex.Replace(builderFile, footPattern, footTarget);
            File.WriteAllText(absoluteBuilderFilePath, builderFile);
            WriteConfigCookie(hasInsertCodeIntoBuilderCookiePath);
            AssetDatabase.Refresh(ImportAssetOptions.ForceUpdate);
        }

        private void SubstitutePrefabFont()
        {
            if (HasConfiged(hasSubstitutePrefabFontCookiePath))
            {
                DisplayWarningDialog("PandoraToolBox prefab 字体已配置");
                return;
            }
#if USING_UGUI
            prefabPath = UGUIPrefabPath;
#endif
#if USING_NGUI
            prefabPath = NGUIPrefabPath;
#endif
            GameObject testPrefab = AssetDatabase.LoadAssetAtPath(prefabPath, typeof(GameObject)) as GameObject;
            GameObject test = PrefabUtility.InstantiatePrefab(testPrefab) as GameObject;
            if (font == null)
            {
                DisplayWarningDialog("未选择字体，请选择字体后再进行配置");
                return;
            }
#if USING_UGUI
            Text[] textComponent = test.GetComponentsInChildren<Text>(true);
            for (int i = 0; i < textComponent.Length; i++)
            {
                textComponent[i].font = font;
            }
#endif
#if USING_NGUI
            UILabel[] labelComponents = test.GetComponentsInChildren<UILabel>(true);
            for (int i = 0; i < labelComponents.Length; i++)
            {
                labelComponents[i].trueTypeFont = font;
            }
#endif
            PrefabUtility.CreatePrefab(prefabPath, test);
            UnityEngine.Object.DestroyImmediate(test);
            WriteConfigCookie(hasSubstitutePrefabFontCookiePath);
            AssetDatabase.Refresh(ImportAssetOptions.ForceUpdate);
        }

        private void SubstituteDJCParams()
        {
            string absolutePandoraToolBoxSettingsFilePath = Path.Combine(Application.dataPath, settingFilePath);
            string testEnvironmentFixedPattern = @"if\s*isTestEnvironment\s*==\s*true\s*then\r\n\s*";
            string fixedPattern = @"\s*=\s*"".*"";";
            string pattern = string.Empty;
            string targetContent;

            if (!File.Exists(absolutePandoraToolBoxSettingsFilePath))
            {
                DisplayWarningDialog(settingFilePath + "不存在，请检查");
                return;
            }

            string testSettingsContent = File.ReadAllText(absolutePandoraToolBoxSettingsFilePath);
            foreach (var item in configDict)
            {
                targetContent = item.Value;
                targetContent = targetContent.Trim();
                if (string.IsNullOrEmpty(targetContent))
                {
                    continue;
                }
                if (!item.Key.Contains("_test"))
                {
                    pattern = item.Key + fixedPattern;
                    targetContent = item.Key + " = \"" + targetContent + "\";";
                }
                else
                {
                    pattern = testEnvironmentFixedPattern + item.Key.Replace("_test", "") + fixedPattern;
                    targetContent = "if isTestEnvironment == true then\r\n\t\t" + item.Key.Replace("_test", "") + " = \"" + targetContent + "\";";
                }
                testSettingsContent = Regex.Replace(testSettingsContent, pattern, targetContent);
            }
            File.WriteAllText(absolutePandoraToolBoxSettingsFilePath, testSettingsContent);
            AssetDatabase.Refresh(ImportAssetOptions.ForceUpdate);
        }

        //cookie 只用于记录是否往Builder.cs中插入了代码，是否替换了prefab的字体
        private void WriteConfigCookie(string cookiePath)
        {
            string absoluteCookiePath = Path.Combine(Application.dataPath, cookiePath);
            File.WriteAllText(absoluteCookiePath, "hasConfiged:true");
        }

        private bool HasConfiged(string cookiePath)
        {
            string absoluteCookiePath = Path.Combine(Application.dataPath, cookiePath);
            if (!File.Exists(absoluteCookiePath))
            {
                return false;
            }
            return true;
        }

        private void DisplayWarningDialog(string message, string title = "")
        {
            EditorUtility.DisplayDialog(title, message, "我知道了");
        }
    }
}