#region HeadComments
// ********************************************************************
//  Copyright (C) #YEAR# #DEVELOPERS#
//  作    者：#AUTHOR#
//  文件路径：#FILEPATH#
//  创建日期：#CREATIONDATE#
//  功能描述：
//
// *********************************************************************
#endregion

using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Collections.Generic;

namespace com.tencent.pandora.tools
{
    public class AutoPlay : EditorWindow
    {
        public const int TOTAL_COUNT = 5;
        private static int _count = 0;
        private static float _lastTime = 0;

        [MenuItem("PandoraTools/AutoPlay")]
        public static void Init()
        {
            _count = 0;
            _lastTime = 0;
            AutoPlay window = EditorWindow.GetWindow<AutoPlay>("自动播放");
            window.Show();
        }

        private void Update()
        {
            if ((Time.realtimeSinceStartup - _lastTime) > 5)
            {
                _lastTime = Time.realtimeSinceStartup;
                Debug.LogError("Playing:  " + EditorApplication.isPlaying.ToString());
                _count += 1;
                EditorApplication.ExecuteMenuItem("Edit/Play");
            }
        }

        private void OnGUI()
        {
            try
            {
                GUILayout.Space(5);
                GUILayout.Label("总次数：");
                GUILayout.Label(TOTAL_COUNT.ToString());
                GUILayout.Label("当前次序:");
                GUILayout.Label(_count.ToString());
            }
            catch
            {

            }
        }


    }
}

