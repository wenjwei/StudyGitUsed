using System.Collections;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

namespace com.tencent.pandora.tools
{
    public static class BuildCommand
    {
        public static void BuildActivity()
        {
            var activity = "";
            var buildTarget = BuildTarget.Android;
            var args = System.Environment.GetCommandLineArgs();
            foreach (var arg in args)
            {
                if (arg.StartsWith("-activity="))
                {
                    activity = arg.Substring(10);
                }

                if (arg.StartsWith("-buildTarget="))
                {
                    var target = arg.Substring(13);
                    buildTarget = target == "android" ? BuildTarget.Android : BuildTarget.iOS;
                }
            }
            Debug.Log("Build activity: " + activity + " target: " + buildTarget.ToString());

            if (string.IsNullOrEmpty(activity))
                return;

            ActivityManager.ToggleActivity(activity, true);
            ActivityManager.ToggleActivityBuildPrefab(activity, true);
            BuilderWindow.Build(buildTarget, false, true);
        }
    }
}