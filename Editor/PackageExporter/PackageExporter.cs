using UnityEditor;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;
using System.Reflection;
using System.IO;

namespace com.tencent.pandora.tools
{
    public class PackageExporter
    {
        [MenuItem("PandoraTools/ExportSDKPackage")]
        public static void Export()
        {
            List<string> paths = new List<string> { 
                                            "Assets/Scripts/GameFramework/PandoraSdkMdl",
                                            "Assets/Scripts/GameFramework/Resources",
                                            "Assets/Plugins/Android",
                                            "Assets/Plugins/CrossPlatformLib/Tencent/Pandora_Managed",
                                            "Assets/Plugins/LibWrapper/External/Slua_Managed",
                                            "Assets/Plugins/MacOS",
                                            "Assets/Plugins/x86_64",
                                            "Assets/Plugins/x86"};

            DirectoryInfo iOSPluginDirectory = new DirectoryInfo("Assets/Plugins/iOS");
            foreach (FileInfo info in iOSPluginDirectory.GetFiles())
            {
                if (info.Name == "PandoraAppController.mm")
                {
                    continue;
                }
                paths.Add("Assets/Plugins/iOS/" + info.Name);
            }

            foreach (DirectoryInfo info in iOSPluginDirectory.GetDirectories())
            {
                paths.Add("Assets/Plugins/iOS/" + info.Name);
            }

            string path = Application.dataPath + "/Plugins/CrossPlatformLib/Tencent/Pandora_Managed/SDK/PandoraSettings.cs";
            string code = File.ReadAllText(path);
            code = code.Replace("public static int DEFAULT_LOG_LEVEL = Logger.DEBUG; //给游戏输出SDK包时替换为Logger.INFO，减少Log对项目组的干扰", "public static int DEFAULT_LOG_LEVEL = Logger.INFO; //给游戏输出SDK包时替换为Logger.INFO，减少Log对项目组的干扰");
            code = code.Replace("return Application.dataPath + \"/CACHE\";", "return Application.dataPath + \"\\\\..\\\\CACHE\";");
            Debug.Log(code);
            File.WriteAllText(path, code);
            AssetDatabase.ExportPackage(paths.ToArray(), "PandoraSDK_SPEEDM_"+ DateTime.Now.ToString("yyyy_MM_dd_HH")  + ".unitypackage", ExportPackageOptions.Recurse);
            code = code.Replace("public static int DEFAULT_LOG_LEVEL = Logger.INFO; //给游戏输出SDK包时替换为Logger.INFO，减少Log对项目组的干扰", "public static int DEFAULT_LOG_LEVEL = Logger.DEBUG; //给游戏输出SDK包时替换为Logger.INFO，减少Log对项目组的干扰");
            code = code.Replace("return Application.dataPath + \"\\\\..\\\\CACHE\";", "return Application.dataPath + \"/CACHE\";");
            File.WriteAllText(path, code);
            
            Debug.Log("打包成功！");
        }
    }

}
