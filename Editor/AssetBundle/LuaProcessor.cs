using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.IO;
using UnityEngine;
using UnityEditor;
using System.Text.RegularExpressions;
using com.tencent.pandora;

namespace com.tencent.pandora.tools
{
    public enum CopyDirection
    {
        DecodeToEncode,
        EncodeToDecode,
    }
    public class LuaProcessor
    {
        //标识是否需要预处理和后处理lua文件
        private static bool IsNeedProcessLua = false;
        //private static bool IsNeedProcessLua = true;
        static Dictionary<string, string> cookieNameAndLuaPathDict = new Dictionary<string, string>()
        {
            { "PandoraToolBoxFunctionConfig.txt", "Actions/Resources/PandoraToolBox/Lua/PandoraToolBoxFunctionConfig.lua.bytes" }, 
            { "PandoraToolBoxProtocolConfig.txt", "Actions/Resources/PandoraToolBox/Lua/PandoraToolBoxProtocolConfig.lua.bytes" }, 
        };
        //需要加解密的lua文件
        private static string[] luaFilePath = new string[] { "Actions/Resources/PandoraToolBox/Lua/PandoraToolBoxReflection.lua.bytes", };
        private static void GenerateToolBoxConfig()
        {
            foreach (var item in cookieNameAndLuaPathDict)
            {
                string config = CookieHelper.Read(item.Key);
                if (string.IsNullOrEmpty(config))
                {
                    return;
                }
                Dictionary<string, object> outerDeserializedConfigDict = MiniJSON.Json.Deserialize(config) as Dictionary<string, object>;
                string luaTableConfig = GenerateOuterLuaTable(outerDeserializedConfigDict);
                string content = "PandoraToolBoxConfig" + luaTableConfig;
                string configPath = Path.Combine(Application.dataPath, item.Value);
                File.WriteAllText(configPath, content);
                AssetDatabase.Refresh(ImportAssetOptions.ForceUpdate);
            }
        }

        private static string GenerateOuterLuaTable(Dictionary<string, object> outerDict)
        {
            List<string> keyList = new List<string>(outerDict.Keys);
            int count = keyList.Count;
            string strLuaTable = string.Empty;
            string innerLuaTable = string.Empty;
            string item = string.Empty;
            for (int i = 0; i < count; i++)
            {
                if (i == 0)
                {
                    strLuaTable = strLuaTable + "{\n";
                }
                innerLuaTable = GenerateInnerLuaTable(outerDict[keyList[i]] as Dictionary<string, object>);
                item = string.Format("\t[\"{0}\"] = {1},\n", keyList[i], innerLuaTable);
                strLuaTable = strLuaTable + item;
                if (i == count - 1)
                {
                    strLuaTable = strLuaTable + "\n}";
                }
            }

            return strLuaTable;
        }

        private static string GenerateInnerLuaTable(Dictionary<string, object> innerDict)
        {
            List<string> keyList = new List<string>(innerDict.Keys);
            int count = keyList.Count;
            string strLuaTable = string.Empty;
            string item = string.Empty;
            for (int i = 0; i < count; i++)
            {
                if (i == 0)
                {
                    strLuaTable = strLuaTable + "{\n";
                }
                item = string.Format("\t\t[\"{0}\"] = \"{1}\",\n", keyList[i], FormatProtocol(innerDict[keyList[i]] as string));
                strLuaTable = strLuaTable + item;
                if (i == count - 1)
                {
                    strLuaTable = strLuaTable + "\t}";
                }
            }
            return strLuaTable;
        }

        private static string FormatProtocol(string originalString)
        {
            //json格式的，要对引号做处理
            if (originalString.Contains("{") && originalString.Contains("}"))
            {
                string regexPattern = @"""";
                originalString = Regex.Replace(originalString, regexPattern, @"\""");
            }
            return originalString;
        }

        public static void PreProcessLuaFile()
        {
            GenerateToolBoxConfig();
            if (!IsNeedProcessLua)
            {
                return;
            }
            CopyLuaFile(CopyDirection.EncodeToDecode);
            DecodeLuaFile();
            AssetDatabase.Refresh(ImportAssetOptions.ForceUpdate);
        }

        public static void PostProcessLuaFile()
        {
            if (!IsNeedProcessLua)
            {
                return;
            }
            DeleteLuaFile();
            AssetDatabase.Refresh(ImportAssetOptions.ForceUpdate);
        }

        private static void CopyLuaFile(CopyDirection direction)
        {
            string sourcePath = string.Empty;
            string destPath = string.Empty;
            for (int i = 0; i < luaFilePath.Length; i++)
            {
                sourcePath = Path.Combine(Application.dataPath, luaFilePath[i]);
                destPath = sourcePath.Replace("Lua", "EncodedFile");
                if (direction == CopyDirection.EncodeToDecode)
                {
                    string middleStr = sourcePath;
                    sourcePath = destPath;
                    destPath = middleStr;
                }

                if (File.Exists(sourcePath))
                {
                    File.Copy(sourcePath, destPath, true);
                }
            }
        }



        private static void DeleteLuaFile()
        {
            string absoluteLuaFilePath = string.Empty;
            for (int i = 0; i < luaFilePath.Length; i++)
            {
                absoluteLuaFilePath = Path.Combine(Application.dataPath, luaFilePath[i]);
                File.Delete(absoluteLuaFilePath);
            }
        }

        //简单加密文件  base64
        private static void EncodeLuaFile()
        {
            string absoluteLuaFilePath = string.Empty;
            for (int i = 0; i < luaFilePath.Length; i++)
            {
                absoluteLuaFilePath = Path.Combine(Application.dataPath, luaFilePath[i]);
                if (File.Exists(absoluteLuaFilePath))
                {
                    string encodedLuaFile = Convert.ToBase64String(File.ReadAllBytes(absoluteLuaFilePath));
                    File.WriteAllText(absoluteLuaFilePath, encodedLuaFile);
                }

            }
        }

        private static void DecodeLuaFile()
        {
            string absoluteLuaFilePath = string.Empty;
            for (int i = 0; i < luaFilePath.Length; i++)
            {
                absoluteLuaFilePath = Path.Combine(Application.dataPath, luaFilePath[i]);
                if (File.Exists(absoluteLuaFilePath))
                {
                    byte[] decodedLuaFile = Convert.FromBase64String(File.ReadAllText(absoluteLuaFilePath));
                    File.WriteAllBytes(absoluteLuaFilePath, decodedLuaFile);
                }

            }
        }


    }
}
