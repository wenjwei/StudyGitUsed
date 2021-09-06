using System;
using UnityEngine;
using UnityEditor;
using System.IO;

namespace com.tencent.pandora.tools
{
    public class RawLoaderSettings : EditorWindow
    {
        [MenuItem("PandoraTools/资源加载模式设置")]
        public static void Init()
        {
            RawLoaderSettings window = EditorWindow.GetWindow<RawLoaderSettings>("Builder");
            window._isLocalMode = Convert.ToBoolean(PlayerPrefs.GetString("RawLoaderSettings.isLocalMode", "false"));
            window.Show();
        }

        private bool _isLocalMode;

        void OnGUI()
        {
            GUILayout.Space(10);

            EditorGUI.BeginChangeCheck();
            _isLocalMode = GUILayout.Toggle(_isLocalMode, new GUIContent("开启快速开发模式", ""));
            if (EditorGUI.EndChangeCheck())
            {
                PlayerPrefs.SetString("RawLoaderSettings.isLocalMode", _isLocalMode.ToString());
                PlayerPrefs.Save();

                var group = BuildPipeline.GetBuildTargetGroup(EditorUserBuildSettings.activeBuildTarget);
                var defineSymbols = PlayerSettings.GetScriptingDefineSymbolsForGroup(group);

                var newDefineSymbols = "";
                if (_isLocalMode)
                {
                    newDefineSymbols = "PANDORA_RAW_MODE;" + defineSymbols;
                }
                else
                {
                    newDefineSymbols = defineSymbols.Replace("PANDORA_RAW_MODE", string.Empty);
                }

                PlayerSettings.SetScriptingDefineSymbolsForGroup(group, newDefineSymbols);

                Debug.Log(string.Format("pre: {0} new: {1}", defineSymbols, newDefineSymbols));
            }
        }
    }
}