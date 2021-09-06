#define USING_NGUI
//#define USING_UGUI
using System;
using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Collections.Generic;
using Object = UnityEngine.Object;
using System.Text.RegularExpressions;
using System.IO;

namespace com.tencent.pandora.tools
{
    public class AtlasMaker_NGUI
    {
#if USING_NGUI
        private const int MAX_ATLAS_SIZE = 2048;
        private const int FAVOR_ATLAS_SIZE = 1024;
        private const int QUALITY_PARAMS = 90;


        private const string SPLIT_SHADER_NAME = "Assets/Plugins/NGUI/Resources/Shaders/Unlit - Transparent Colored.shader";
        private static Regex SCALE_9_EXP = new Regex(@"(?<=@)l\d+_r\d+_t\d+_b\d+");
        private static Regex SCALE_9_LEFT = new Regex(@"(?<=l)\d+(?=_)");
        private static Regex SCALE_9_RIGHT = new Regex(@"(?<=r)\d+(?=_)");
        private static Regex SCALE_9_TOP = new Regex(@"(?<=t)\d+(?=_)");
        private static Regex SCALE_9_BOTTOM = new Regex(@"(?<=b)\d+");
        private const string BIG_TEXTURE_SLICE = "_@slice_";

        private static Texture2D _atlas;

        [MenuItem("PandoraTools/MakeAtlas_NGUI")]
        [MenuItem("Assets/MakeAtlas_NGUI")]
        public static void MakeAtlas()
        {
            //选择贴图列表
            List<string> texturePathList = SelectTexturePathList();
            MakeAtlas(texturePathList);
        }

        public static string MakeAtlas(List<string> texturePathList)
        {
            string atlasPath = null;
            if (texturePathList.Count > 0)
            {
                atlasPath = GetAtlasPath(texturePathList[0]);
            }
            if (string.IsNullOrEmpty(atlasPath) == true)
            {
                EditorUtility.DisplayDialog("消息", "未选择任何路径", "OK");
                return string.Empty;
            }
            GameObject existAtlas = AssetDatabase.LoadAssetAtPath(atlasPath, typeof(GameObject)) as GameObject;
            if (existAtlas != null)
            {
                //更新Atlas：以目录下最新图片列表为准，增加，删除、更新Sprite，已有的Sprite GUID不改变
                RefreshAtlas(atlasPath, texturePathList);
            }
            else
            {
                //创建新的Atlas
                CreateAtlas(atlasPath, texturePathList);
            }
            return atlasPath;
        }

        private static List<string> SelectTexturePathList()
        {
            List<string> result = new List<string>();
            Object[] objs = Selection.GetFiltered(typeof(Texture2D), SelectionMode.Assets);
            if (objs.Length == 0)
            {
                EditorUtility.DisplayDialog("错误", "选择的图片长度为0", "ok");
                return result;
            }
            foreach (Object o in objs)
            {
                result.Add(AssetDatabase.GetAssetPath(o));
            }
            return result;
        }

        /// <summary>
        /// 返回Assets\开头的资源路径
        /// </summary>
        /// <returns></returns>
        private static string GetAtlasPath(string texturePath)
        {
            string textureFolderPath = Path.GetDirectoryName(texturePath);
            string actionFolderPath = Path.GetDirectoryName(textureFolderPath);
            string actionName = Path.GetFileNameWithoutExtension(actionFolderPath);
            string atlasName = actionName + "_Atlas.prefab";
            string atlasPath = Path.Combine(Path.Combine(actionFolderPath, "Atlas"), atlasName);
            string path = EditorUtility.SaveFilePanel("Save Atlas", Application.dataPath + atlasPath.Replace("Assets", ""), atlasName, "prefab");
            return path.Replace(Application.dataPath, "Assets");
        }

        private static void RefreshAtlas(string atlasPath, List<string> texturePathList)
        {
            Dictionary<string, UISpriteData> spriteDataDict = GetSpriteDataDict(atlasPath);
            string rgbAtlasPath = atlasPath.Replace(".prefab", "_rgb.png");
            string alphaAtlasPath = atlasPath.Replace(".prefab", "_alpha.png");
            string rgbaAtlasPath = atlasPath.Replace(".prefab", ".png");
            string materialPath = atlasPath.Replace(".prefab", "_mat.mat");
            _atlas = new Texture2D(MAX_ATLAS_SIZE, MAX_ATLAS_SIZE);
            _atlas.name = Path.GetFileNameWithoutExtension(atlasPath);
            Rect[] rects = _atlas.PackTextures(GetPackTextures(texturePathList), 0, MAX_ATLAS_SIZE, false);
            _atlas = AtlasOptimizer.Optimize(_atlas, rects, true);
            AtlasWriter.Write(_atlas, rgbaAtlasPath);
            ImportRawAtlasTexture(rgbaAtlasPath);
            ImageChannelSpliter.Execute(rgbaAtlasPath, rgbAtlasPath, alphaAtlasPath);
            //AssetDatabase.DeleteAsset(rgbaAtlasPath);
            ImportChannelTexture(alphaAtlasPath);
            ImportChannelTexture(rgbAtlasPath);
            CreateMaterial(SPLIT_SHADER_NAME, rgbAtlasPath, alphaAtlasPath);
            GameObject prefab = AssetDatabase.LoadAssetAtPath(atlasPath, typeof(GameObject)) as GameObject;
            GameObject go = PrefabUtility.InstantiatePrefab(prefab as Object) as GameObject;
            UIAtlas atlas = go.GetComponent<UIAtlas>();
            atlas.spriteMaterial = AssetDatabase.LoadAssetAtPath(materialPath, typeof(Material)) as Material;
            atlas.spriteList = GetSpriteDataList(rects, GetPackTextureNames(texturePathList), GetPackTextureBorders(texturePathList), _atlas.width, _atlas.height, spriteDataDict);
            atlas.MarkAsChanged();
            PrefabUtility.ReplacePrefab(go, prefab);
            GameObject.DestroyImmediate(go);
            AssetDatabase.SaveAssets();
            AssetDatabase.ImportAsset(atlasPath, ImportAssetOptions.ForceUpdate);
            ForceRefreshAllSprites();
            LogAtlasSize(_atlas);
            EditorUtility.DisplayDialog("提示", string.Format("图集更新成功, {0} 宽：{1} 高：{2}", _atlas.name, _atlas.width, _atlas.height), "知道了~");
        }

        private static void ForceRefreshAllSprites()
        {
            UISprite[] sprites = GameObject.FindObjectsOfType(typeof(UISprite)) as UISprite[];
            for (int i = 0; i < sprites.Length; i++)
            {
                UISprite sprite = sprites[i];
                sprite.enabled = false;
                sprite.enabled = true;
            }
        }

        public static void CreateAtlas(string atlasPath, List<string> texturePathList)
        {
            _atlas = new Texture2D(MAX_ATLAS_SIZE, MAX_ATLAS_SIZE);
            _atlas.name = Path.GetFileNameWithoutExtension(atlasPath);
            string rgbAtlasPath = atlasPath.Replace(".prefab", "_rgb.png");
            string alphaAtlasPath = atlasPath.Replace(".prefab", "_alpha.png");
            string rgbaAtlasPath = atlasPath.Replace(".prefab", ".png");
            string materialPath = atlasPath.Replace(".prefab", "_mat.mat");
            Rect[] rects = _atlas.PackTextures(GetPackTextures(texturePathList), 0, MAX_ATLAS_SIZE, false);
            _atlas = AtlasOptimizer.Optimize(_atlas, rects, true);
            AtlasWriter.Write(_atlas, rgbaAtlasPath);
            ImportRawAtlasTexture(rgbaAtlasPath);
            ImageChannelSpliter.Execute(rgbaAtlasPath, rgbAtlasPath, alphaAtlasPath);
            AssetDatabase.DeleteAsset(rgbaAtlasPath);
            ImportChannelTexture(alphaAtlasPath);
            ImportChannelTexture(rgbAtlasPath);
            CreateMaterial(SPLIT_SHADER_NAME, rgbAtlasPath, alphaAtlasPath);
            string name = Path.GetFileNameWithoutExtension(atlasPath);
            GameObject go = new GameObject(name);
            UIAtlas atlas = go.AddComponent<UIAtlas>();
            atlas.spriteMaterial = AssetDatabase.LoadAssetAtPath(materialPath, typeof(Material)) as Material;
            atlas.spriteList = GetSpriteDataList(rects, GetPackTextureNames(texturePathList), GetPackTextureBorders(texturePathList), _atlas.width, _atlas.height, null);
            PrefabUtility.CreatePrefab(atlasPath, go);
            AssetDatabase.SaveAssets();
            GameObject.DestroyImmediate(go);
            LogAtlasSize(_atlas);
            EditorUtility.DisplayDialog("提示", string.Format("图集创建成功, {0} 宽：{1} 高：{2}", _atlas.name, _atlas.width, _atlas.height), "知道了~");
        }

        //更新Atlas时记录已有Sprite的设置参数
        private static Dictionary<string, UISpriteData> GetSpriteDataDict(string atlasPath)
        {
            Dictionary<string, UISpriteData> result = new Dictionary<string, UISpriteData>();
            GameObject atlasPrefab = AssetDatabase.LoadAssetAtPath(atlasPath,typeof(GameObject)) as GameObject;
            UIAtlas atlas = atlasPrefab.GetComponent<UIAtlas>();
            for(int i = 0; i < atlas.spriteList.Count; i++)
            {
                UISpriteData spriteData = atlas.spriteList[i];
                if(result.ContainsKey(spriteData.name) == false)
                {
                    result.Add(spriteData.name, spriteData);
                }
            }
            return result;
        }

        //nGUI sprite data
        private static List<UISpriteData> GetSpriteDataList(Rect[] rects, string[] names, Vector4[] borders, int width, int height, Dictionary<string, UISpriteData> spriteDataDict)
        {
            List<UISpriteData> spriteDataList = new List<UISpriteData>();
            for (int i = 0; i < rects.Length; i++)
            {
                Rect rect = rects[i];
                Vector4 border = borders[i];
                UISpriteData spriteData = new UISpriteData();
                spriteData.name = names[i];
                spriteData.x = (int)(rect.xMin * width) + TextureClamper.BORDER;
                spriteData.y = (int)((1 - rect.yMax) * height) + TextureClamper.BORDER;
                spriteData.width = (int)(rect.width * width) - TextureClamper.BORDER * 2;
                spriteData.height = (int)(rect.height * height) - TextureClamper.BORDER * 2;
                if (spriteDataDict != null && spriteDataDict.ContainsKey(spriteData.name) == true)
                {
                    UISpriteData recordSpriteData = spriteDataDict[spriteData.name];
                    spriteData.borderLeft = recordSpriteData.borderLeft;
                    spriteData.borderBottom = recordSpriteData.borderBottom;
                    spriteData.borderRight = recordSpriteData.borderRight;
                    spriteData.borderTop = recordSpriteData.borderTop;
                }
                if (spriteData.borderLeft == 0 && spriteData.borderBottom == 0 && spriteData.borderRight == 0 && spriteData.borderTop == 0)
                {
                    spriteData.borderLeft = (int)border.x;
                    spriteData.borderBottom = (int)border.y;
                    spriteData.borderRight = (int)border.z;
                    spriteData.borderTop = (int)border.w;
                }
                spriteDataList.Add(spriteData);
            }
            return spriteDataList;
        }

        private static void ImportRawAtlasTexture(string path)
        {
            AssetDatabase.ImportAsset(path, ImportAssetOptions.ForceUpdate);
            TextureImporter importer = AssetImporter.GetAtPath(path) as TextureImporter;
            importer.maxTextureSize = MAX_ATLAS_SIZE;
            importer.spriteImportMode = SpriteImportMode.None;
            importer.mipmapEnabled = false;
            importer.isReadable = true;
            importer.filterMode = FilterMode.Point;
            SetTextureCompressFormat(importer, TextureImporterFormat.RGBA32, TextureImporterFormat.RGBA32);
            AssetDatabase.ImportAsset(path, ImportAssetOptions.ForceUpdate);
        }

        private static void ImportChannelTexture(string path)
        {
            AssetDatabase.ImportAsset(path, ImportAssetOptions.ForceUpdate);
            TextureImporter importer = AssetImporter.GetAtPath(path) as TextureImporter;
            importer.maxTextureSize = MAX_ATLAS_SIZE;
            importer.spriteImportMode = SpriteImportMode.None;
            importer.mipmapEnabled = false;
            importer.isReadable = false;
            SetTextureCompressFormat(importer, TextureImporterFormat.ETC_RGB4, TextureImporterFormat.PVRTC_RGB4);
            AssetDatabase.ImportAsset(path, ImportAssetOptions.ForceUpdate);
        }

        private static void SetTextureCompressFormat(TextureImporter importer, TextureImporterFormat androidFormat, TextureImporterFormat iPhoneFormat)
        {
#if UNITY_4_6 || UNITY_4_7
            importer.textureFormat = TextureImporterFormat.RGBA32;
            importer.SetPlatformTextureSettings("iPhone", MAX_ATLAS_SIZE, iPhoneFormat, QUALITY_PARAMS);
            importer.SetPlatformTextureSettings("Android", MAX_ATLAS_SIZE, androidFormat, QUALITY_PARAMS);
#elif UNITY_5
            importer.textureFormat = TextureImporterFormat.RGBA32;
            importer.SetPlatformTextureSettings("iPhone", MAX_ATLAS_SIZE, iPhoneFormat, QUALITY_PARAMS, false);
            importer.SetPlatformTextureSettings("Android", MAX_ATLAS_SIZE, androidFormat, QUALITY_PARAMS, false);
#elif UNITY_2017_1_OR_NEWER
            importer.textureCompression = TextureImporterCompression.Uncompressed;
            TextureImporterPlatformSettings platFormatAndroid = importer.GetPlatformTextureSettings("Android");
            platFormatAndroid.name = "Android";
            platFormatAndroid.format = androidFormat;
            platFormatAndroid.overridden = true;

            TextureImporterPlatformSettings platFormatIos = importer.GetPlatformTextureSettings("iPhone");
            platFormatIos.name = "iPhone";
            platFormatIos.format = iPhoneFormat;
            platFormatIos.overridden = true;

            importer.allowAlphaSplitting = false;
            importer.maxTextureSize = MAX_ATLAS_SIZE;
            importer.compressionQuality = QUALITY_PARAMS;
            
            importer.SetPlatformTextureSettings(platFormatAndroid);
            importer.SetPlatformTextureSettings(platFormatIos);
#endif
        }

        public static void CreateMaterial(string shaderPath, string texturePath, string alphaTexturePath)
        {
            Shader shader = AssetDatabase.LoadAssetAtPath(shaderPath, typeof(Shader)) as Shader;
            Material material = new Material(shader);
            Texture2D texture = AssetDatabase.LoadAssetAtPath(texturePath, typeof(Texture2D)) as Texture2D;
            Texture2D alphaTexture = AssetDatabase.LoadAssetAtPath(alphaTexturePath, typeof(Texture2D)) as Texture2D;
            material.SetTexture("_MainTex", texture);
            material.SetTexture("_AlphaTex", alphaTexture);
            string materialPath = texturePath.Replace("_rgb.png", "_mat.mat");
            if (File.Exists(materialPath) == true)
            {
                AssetDatabase.DeleteAsset(materialPath);
            }
            AssetDatabase.CreateAsset(material, materialPath);
            AssetDatabase.SaveAssets();
        }

        private static Texture2D[] GetPackTextures(List<string> pathList)
        {
            Texture2D[] result = new Texture2D[pathList.Count];
            for (int i = 0; i < result.Length; i++)
            {
                string path = pathList[i];
                Texture2D texture = AssetDatabase.LoadAssetAtPath(path, typeof(Texture2D)) as Texture2D;
                if (path.Contains(BIG_TEXTURE_SLICE) == false) //大图切片已经增加过2像素
                {
                    texture = TextureClamper.Clamp(texture);
                }
                result[i] = texture;
            }
            return result;
        }

        private static string[] GetPackTextureNames(List<string> pathList)
        {
            string[] result = new string[pathList.Count];
            for (int i = 0; i < pathList.Count; i++)
            {
                result[i] = Path.GetFileNameWithoutExtension(pathList[i]);
            }
            return result;
        }

        private static Vector4[] GetPackTextureBorders(List<string> pathList)
        {
            Vector4[] result = new Vector4[pathList.Count];
            for (int i = 0; i < pathList.Count; i++)
            {
                string name = Path.GetFileNameWithoutExtension(pathList[i]);
                if (SCALE_9_EXP.IsMatch(name) == true)
                {
                    string nameMatchResult = SCALE_9_EXP.Match(name).Value;
                    string left = SCALE_9_LEFT.Match(nameMatchResult).Value;
                    string right = SCALE_9_RIGHT.Match(nameMatchResult).Value;
                    string top = SCALE_9_TOP.Match(nameMatchResult).Value;
                    string bottom = SCALE_9_BOTTOM.Match(nameMatchResult).Value;
                    result[i] = new Vector4(int.Parse(left), int.Parse(bottom), int.Parse(right), int.Parse(top));
                }
                else
                {
                    result[i] = Vector4.zero;
                }
            }
            return result;
        }

        private static void LogAtlasSize(Texture2D atlas)
        {
            if (atlas.width >= MAX_ATLAS_SIZE || atlas.height >= MAX_ATLAS_SIZE)
            {
                string content = atlas.name + " 图集宽高尺寸超过1024像素，为 " + atlas.width + " 像素，可以通过重新组织为2个图集的方式来优化。";
                EditorUtility.DisplayDialog("图集尺寸超标", content, "改改改~");
                Debug.LogError(content);
            }
            else
            {
                Debug.Log(string.Format("<color=#00ff00>图集 {0} 尺寸为： {1}x{2}</color>", atlas.name, atlas.width, atlas.height));
            }
        }

#endif
    }
}