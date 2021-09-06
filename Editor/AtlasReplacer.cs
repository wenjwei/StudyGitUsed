using System;
using System.IO;
using System.Text;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

namespace com.tencent.pandora.tools
{
    /// <summary>
    /// 检查活动Prefab对飞车公共图集使用情况
    /// </summary>
    public class AtlasUsageInspector
    {
        private static StringBuilder _logBuilder;

        [MenuItem("PandoraTools/AtlasUsageInspector")]
        public static void Main()
        {
            _logBuilder = new StringBuilder();
            List<string> prefabPathList = GetPrefabPathList();
            foreach(string path in prefabPathList)
            {
                CheckPrefab(path);
            }

            File.WriteAllText(Path.Combine(Application.dataPath, "AtlasReport.txt"), _logBuilder.ToString());
            Debug.Log("结果写入： " + Path.Combine(Application.dataPath, "AtlasReport.txt"));
        }

        private static List<string> GetPrefabPathList()
        {
            string[] guids = AssetDatabase.FindAssets("t:GameObject", new string[] { "Assets/Actions/Resources" });
            List<string> result = new List<string>(guids.Length);
            foreach(string g in guids)
            {
                result.Add(AssetDatabase.GUIDToAssetPath(g));
            }
            return result;
        }

        private static void CheckPrefab(string path)
        {
            _logBuilder.Append("============================Prefab: ");
            _logBuilder.Append(path); _logBuilder.Append("\n");
            GameObject go = AssetDatabase.LoadAssetAtPath(path, typeof(GameObject)) as GameObject;
            UISprite[] sprites = go.GetComponentsInChildren<UISprite>(true);
            foreach(UISprite s in sprites)
            {
                if(s.atlas != null)
                {
                    if(s.atlas.name == "C_FG" || s.atlas.name == "C_BG")
                    {
                        string content = string.Format("    {0} {1} {2}", s.gameObject.name, s.atlas.name, s.spriteName);
                        _logBuilder.Append(content); _logBuilder.Append("\n");
                    }
                }
            }
        }
    }
}

