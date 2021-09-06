using UnityEngine;
using UnityEditor;
using System;
using System.Collections.Generic;
using UnityEditor.Callbacks;

namespace com.tencent.pandora.tools
{
    public class BuilderWindow : EditorWindow
    {
        private static GUIStyle FONT_BOLD_STYLE;
        private static Color COLOR_GREEN = new Color(26.0f / 255.0f, 171.0f / 255.0f, 37.0f / 255.0f);

        private static Vector2 scroll = Vector2.zero;

        [MenuItem("PandoraTools/BuildAssets &#b")]
        public static void Init()
        {
            ActivityManager.Refresh();
            BuilderWindow window = EditorWindow.GetWindow<BuilderWindow>("Builder");
            window.Show();
        }

        [DidReloadScripts]
        private static void OnScriptReload()
        {
            ActivityManager.Refresh();
        }

        private void OnEnable()
        {
            FONT_BOLD_STYLE = new GUIStyle() { fontStyle = FontStyle.Bold };
        }

        void OnGUI()
        {
            try
            {
                GUILayout.Space(5);
                GUILayout.Label("Please select activities:", FONT_BOLD_STYLE);
                ShowActivityList();
                ShowBuildItem("Build Current & Play", GetBuildTarget(), true);
                
                ShowBuildItem("Build Android", BuildTarget.Android);
#if UNITY_5 || UNITY_2017_1_OR_NEWER
                ShowBuildItem("Build iOS", BuildTarget.iOS);
#else
                ShowBuildItem("Build iPhone", BuildTarget.iOS);
#endif
                ShowBuildItem("Build PC", BuildTarget.StandaloneWindows);
                Event e = Event.current;
                if (e.type == EventType.KeyDown && e.keyCode == KeyCode.B)
                {
                    Build(GetBuildTarget(), true, false);
                }
            }
            catch (Exception e)
            {
                Debug.LogException(e);
            }
        }

        private static BuildTarget GetBuildTarget()
        {
            switch(EditorUserBuildSettings.activeBuildTarget)
            {
                case BuildTarget.StandaloneWindows:
                case BuildTarget.StandaloneWindows64:
                    return BuildTarget.StandaloneWindows;
                case BuildTarget.Android:
                    return BuildTarget.Android;
#if UNITY_5 || UNITY_2017_1_OR_NEWER
                case BuildTarget.iOS:
                case BuildTarget.StandaloneOSX:
                    return BuildTarget.iOS;
#else
                case BuildTarget.iOS:
                case BuildTarget.StandaloneOSX:
                    return BuildTarget.iOS;
#endif
            }
            return BuildTarget.Android;
        }

        private static void ShowActivityList()
        {
            scroll = GUILayout.BeginScrollView(scroll);
            List<string> nameList = ActivityManager.GetActivityNameList();
            for (int i = 0; i < nameList.Count; i++)
            {
                string name = nameList[i];
                GUILayout.BeginHorizontal();
                bool toggle = EditorGUILayout.Toggle(name, ActivityManager.IsActivitySelected(name));
                ActivityManager.ToggleActivity(name, toggle);
                //�Ƿ���Prefab��Դ��������unity4.x�£�ÿ�δ��Prefab
                bool buildPrefab = EditorGUILayout.Toggle("Build Prefab", ActivityManager.IsActivityBuildPrefab(name));
                ActivityManager.ToggleActivityBuildPrefab(name, buildPrefab);
                GUILayout.EndHorizontal();
            }
            GUILayout.EndScrollView();
            GUILayout.Space(5);
        }

        private static void ShowBuildItem(string label, BuildTarget target, bool playAfterBuild = false)
        {
            Color color = GUI.backgroundColor;
            GUI.backgroundColor = COLOR_GREEN;
            if (GUILayout.Button(label, GUILayout.Height(30)))
            {
                Build(target, playAfterBuild, true);
            }
            GUI.backgroundColor = color;
        }

        public static void Build(BuildTarget target, bool playAfterBuild = false, bool showReport = true)
        {
            List<String> selectedList = ActivityManager.GetSelectedActivityNameList();
            Builder.Build(selectedList, target, showReport);
            if (playAfterBuild == true)
            {
                EditorApplication.isPlaying = true;
            }
        }

    }
}
