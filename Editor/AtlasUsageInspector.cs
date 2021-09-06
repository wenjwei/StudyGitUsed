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
    public class AtlasReplacer
    {
        private static StringBuilder _logBuilder;
        private static UIAtlas _maskAtlas;

        [MenuItem("PandoraTools/AtlasReplacer")]
        public static void Main()
        {
            _maskAtlas = AssetDatabase.LoadAssetAtPath("Assets/UI/Atlas/C_Mask/C_Mask.prefab", typeof(UIAtlas)) as UIAtlas;
            _logBuilder = new StringBuilder();
            List<string> prefabPathList = GetPrefabPathList();
            foreach(string path in prefabPathList)
            {
                if(path.Contains("/Prefabs/"))
                {
                    CheckPrefab(path);
                }
            }
            AssetDatabase.SaveAssets();
            File.WriteAllText(Path.Combine(Application.dataPath, "AtlasReplaceReport.txt"), _logBuilder.ToString());
            Debug.Log("结果写入： " + Path.Combine(Application.dataPath, "AtlasReplaceReport.txt"));
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
            GameObject prefab = AssetDatabase.LoadAssetAtPath(path, typeof(GameObject)) as GameObject;
            GameObject go = PrefabUtility.InstantiatePrefab(prefab) as GameObject;
            UISprite[] sprites = go.GetComponentsInChildren<UISprite>(true);
            bool isDirty = false;
            foreach(UISprite s in sprites)
            {
                if(s.atlas != null)
                {
                    if(s.atlas.name == "C_FG")
                    {
                        if(s.spriteName == "Dec_Light_01")
                        {
                            string content = string.Format("    {0} {1} {2}", s.gameObject.name, s.atlas.name, s.spriteName);
                            _logBuilder.Append(content); _logBuilder.Append("\n");
                            s.atlas = _maskAtlas;
                            s.spriteName = "Dec_FG_Light_01";
                            isDirty = true;
                        }
                        else if(s.spriteName == "Icon_King")
                        {
                            string content = string.Format("    {0} {1} {2}", s.gameObject.name, s.atlas.name, s.spriteName);
                            _logBuilder.Append(content); _logBuilder.Append("\n");
                            s.atlas = _maskAtlas;
                            s.spriteName = "Icon_FG_King";
                            isDirty = true;
                        }
                    }
                }
            }
            if(isDirty == true)
            {
                PrefabUtility.ReplacePrefab(go, prefab);
            }
            else
            {
                GameObject.DestroyImmediate(go);
            }
        }
    }
}

