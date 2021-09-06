
using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;

namespace com.tencent.pandora.tools
{
    public class EmbeddedResConfig
    {
        [MenuItem("PandoraTools/生成内置资源配置文件")]
        public static void Config()
        {
            var path = Path.Combine(Application.streamingAssetsPath, "Pandora");
            var res = Directory.GetFiles(path, "*.assetbundle");
            var confDict = new DictionaryView<string, string>();
            foreach (var re in res)
            {
                var resName = Path.GetFileName(re);
                var resMd5 = CachedFileMd5Helper.GetFileMd5(re);
                confDict.Add(resName, resMd5);
                Debug.Log(string.Format("处理文件: {0}, md5: {1}", resName, resMd5));
            }

            if (confDict.Count > 0)
            {
                var configPath = Path.Combine(path, "embeddedResConfig.txt");
                if (File.Exists(configPath))
                    File.Delete(configPath);
                var fs = File.Create(configPath);
                var ws = new StreamWriter(fs);
                ws.Write(MiniJSON.Json.Serialize(confDict));
                ws.Close();
                fs.Close();
                Debug.Log(string.Format("已处理{0}个文件，配置文件生成路径：{1}", res.Length, configPath));
                AssetDatabase.Refresh();
            }
            else
            {
                Debug.Log("没有文件处理");
            }
        }
    }
}