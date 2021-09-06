using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using System.IO;

/// <summary>
/// 启动时检查工程目录下.git文件中pre-commit文件是否存在，不存在的话
/// 从GitHookScripts目录下复制一份到此处
/// </summary>
namespace com.tencent.pandora.tools
{
    [InitializeOnLoad] //启动Unity时运行
    public class GitHookInstaller
    {
        static string BASH_HEAD = "#!/bin/sh\n\n";
        static string TOKEN = "#仅允许提交Assets/Actions/Resources/目录下文件";
        static string CONTENT = ""
        + "#仅允许提交Assets/Actions/Resources/目录下文件"
        + "\nchangedPaths=\"$(git diff --cached --name-only)\""
        + "\nfor path in $changedPaths"
        + "\ndo"
	    +    "\n\tif [[ $path != *\"Assets/Actions/Resources/\"* ]]; then"
  		+        "\n\t\techo \"提交失败！含有活动目录外的文件(Assets/Actions/Resources)，请取消相关文件再提交。\""
  		+        "\n\t\techo $path"
  		+        "\n\t\techo "
  		+        "\n\t\texit 1"
	    +    "\n\tfi"
        + "\ndone";

        static GitHookInstaller()
        {
            Main();
        }

        public static void Main()
        {
            string basePath = Application.dataPath.Replace("/Assets", "");
            string targetPath = basePath + "/.git/hooks/pre-commit";
            if(File.Exists(targetPath) == false)
            {
                File.WriteAllText(targetPath, BASH_HEAD);
                File.AppendAllText(targetPath, CONTENT);
            }
            else
            {
                string content = File.ReadAllText(targetPath);
                if(content.Contains(TOKEN) == false)
                {
                    File.AppendAllText(targetPath, CONTENT);
                }
            }
        }
    }
}

