using UnityEditor;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;
using System.Reflection;

namespace com.tencent.pandora.tools
{
    public class ProjectBuilder : Editor
    {
        public static void Build()
        {
            string outputPath = string.Empty;
            try
            {
                int position = -1;
                string[] lines = Environment.GetCommandLineArgs();
                for (int i = 0; i < lines.Length; i++)
                {
                    if (lines[i] == "-output")
                    {
                        position = i;
                        break;
                    }
                }

                if((position + 1) < lines.Length)
                {
                    outputPath = lines[position + 1];
                }
            }
            catch(Exception e)
            {
                Debug.LogException(e);
                Debug.LogError("参数设置错误");
            }

            if(string.IsNullOrEmpty(outputPath))
            {
                outputPath = string.Concat(Application.dataPath.Replace("/Assets", "/"), "Build/PandoraUnityDemo.apk");
            }
            string[] outScenes = new string[] { "Assets/Scene/Demo.unity" };
            BuildPipeline.BuildPlayer(outScenes, outputPath, BuildTarget.Android, BuildOptions.None);

        }
    }

}
