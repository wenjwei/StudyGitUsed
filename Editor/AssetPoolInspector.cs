using UnityEditor;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;
using System.Reflection;

namespace com.tencent.pandora.tools
{
    public class AssetPoolInspector : EditorWindow
    {
        private static AssetPoolInspector _window;

        private Vector2 _scrollPosition;
        private GUIStyle _defaultStyle;

        [MenuItem("PandoraTools/AssetPoolInspector")]
        public static void Main()
        {
            _window = EditorWindow.GetWindow<AssetPoolInspector>("AssetPoolInspector");
            _window.Show();

        }

        private void OnGUI()
        {
            if (Application.isPlaying)
            {
                try
                {
                    Refresh();
                }
                catch (Exception e)
                {
                    Debug.LogException(e);
                }
            }
        }

        private void Refresh()
        {
            Type type = GetPandoraType();
            if (type == null) return;
            _defaultStyle = new GUIStyle();
            _defaultStyle.richText = true;
            ShowLuaUsedMemory(type);
            GUILayout.Space(5);
            _scrollPosition = EditorGUILayout.BeginScrollView(_scrollPosition);
            ShowPrefabTextureSet(type);
            ShowAssetReferenceCount(type);
            EditorGUILayout.EndScrollView();
        }

        private void ShowLuaUsedMemory(Type type)
        {
            MethodInfo GetLuaUsedMemory = type.GetMethod("GetLuaUsedMemory", BindingFlags.Static | BindingFlags.Public);
            GUILayout.Label("<color=#ffff00>Pandora Lua内存使用量： " + GetLuaUsedMemory.Invoke(null, null).ToString() + " KB</color>", _defaultStyle);
        }

        private void ShowPrefabTextureSet(Type type)
        {
            Color recordColor = GUI.backgroundColor;
            GUI.backgroundColor = Color.green;
            GUILayout.Button("=================Prefab贴图资源依赖信息=================");
            GUI.backgroundColor = recordColor;
            MethodInfo GetPrefabTextureSetDict = type.GetMethod("GetPrefabTextureSetDict", BindingFlags.Static | BindingFlags.Public);
            if (GetPrefabTextureSetDict != null)
            {
                Dictionary<string, HashSet<Texture>> prefabTextureSetDict = GetPrefabTextureSetDict.Invoke(null, null) as Dictionary<string, HashSet<Texture>>;
                foreach (var kvp in prefabTextureSetDict)
                {
                    GUILayout.BeginHorizontal();
                    GUILayout.Label("<color=#ffff00>" + kvp.Key + "</color>", _defaultStyle);
                    GUILayout.EndHorizontal();
                    foreach (var v in kvp.Value)
                    {
                        GUILayout.BeginHorizontal();
                        GUILayout.Label("    ", GUILayout.Width(50));
                        string color = v.width > 1024 ? "ff0000" : "ffff00";
                        GUILayout.Label(string.Format("<color=#{5}>贴图名称: <b>{0}</b>，尺寸：<b>{1}x{2}</b>  压缩格式：<b>{3}</b>，占用内存:<b>{4}</b></color>", v.name, v.width, v.height, (v as Texture2D).format.ToString(), GetTextureOccupiedMemory(v as Texture2D), color), _defaultStyle);
                        GUILayout.EndHorizontal();
                    }
                }
            }
        }

        private string GetTextureOccupiedMemory(Texture2D texture)
        {
            byte[] bytes = texture.GetRawTextureData();
            return ((float)(bytes.Length) / 1024.0f / 1024.0f).ToString() + "M";
        }

        private void ShowAssetReferenceCount(Type type)
        {
            Color recordColor = GUI.backgroundColor;
            GUI.backgroundColor = Color.green;
            GUILayout.Button("=================Active Assets(不包含不缓存的资源)=================");
            GUI.backgroundColor = recordColor;
            MethodInfo GetAssetReferenceCountDict = type.GetMethod("GetAssetReferenceCountDict", BindingFlags.Static | BindingFlags.Public);
            Dictionary<string, int> assetReferenceCountDict = GetAssetReferenceCountDict.Invoke(null, null) as Dictionary<string, int>;
            foreach (string key in assetReferenceCountDict.Keys)
            {
                GUILayout.BeginHorizontal();
                GUI.backgroundColor = Color.yellow;
                GUILayout.Button("Asset", GUILayout.Width(50));
                int count = 0;
                assetReferenceCountDict.TryGetValue(key, out count);
                GUI.backgroundColor = Color.gray;
                if (count == 0)
                {
                    GUI.backgroundColor = Color.red;
                }
                GUILayout.Button("引用次数: " + count.ToString(), GUILayout.Width(100));
                GUI.backgroundColor = recordColor;
                GUILayout.Label(key);
                GUILayout.EndHorizontal();
            }

        }

        private Type GetPandoraType()
        {
            Assembly[] assemblies = AppDomain.CurrentDomain.GetAssemblies();
            for (int i = 0; i < assemblies.Length; i++)
            {
                Assembly assembly = assemblies[i];
                Type type = assembly.GetType("com.tencent.pandora.Pandora");
                if (type != null)
                {
                    return type;
                }
            }
            return null;
        }

    }
}

