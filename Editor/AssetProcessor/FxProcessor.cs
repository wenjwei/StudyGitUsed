using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;

namespace com.tencent.pandora.tools
{
    /// <summary>
    /// 提取选中预制体上的材质，将其加入到MaterialHook中，以达到共享游戏内Material的目的
    /// </summary>
    public class FxProcessor
    {
        const string MaterialHookPath = "Config/Pandora/MaterialHook";

        [MenuItem("PandoraTools/Fx/Add MaterialPartner", false, 1012)]
        private static void AddMaterialPartner()
        {
            GameObject fxObj = PrefabUtility.InstantiatePrefab(Selection.activeGameObject) as GameObject;
            Renderer[] renders = fxObj.GetComponentsInChildren<Renderer>(true);
            foreach (Renderer r in renders)
            {
                if (r.gameObject.GetComponent<MaterialPartner>() != null)
                {
                    Debug.LogError(string.Format("预制体:{0} <color=#00ff00>节点:{1}</color> <color=#ff0000>已存在MaterialPartner组件</color>", fxObj.name, r.gameObject.name));
                    continue;
                }

                MaterialPartner partner = r.gameObject.AddComponent<MaterialPartner>();
                if (partner != null)
                {
                    Debug.Log(string.Format("预制体:{0} <color=#00ff00>节点:{1}</color> <color=#ff0000>添加MaterialPartner</color>", fxObj.name, r.gameObject.name));
                }
                else
                {
                    Debug.LogError(string.Format("预制体:{0} <color=#00ff00>节点:{1}</color> <color=#ff0000>添加MaterialPartner失败</color>", fxObj.name, r.gameObject.name));
                }
            }

            PrefabUtility.ReplacePrefab(fxObj, Selection.activeGameObject, ReplacePrefabOptions.ReplaceNameBased);

            GameObject.DestroyImmediate(fxObj);
        }

        [MenuItem("PandoraTools/Fx/Add to MaterialHook", false, 1013)]
        private static void AddToMaterialHook()
        {
            Dictionary<Material, GameObject> collectMaterials = new Dictionary<Material, GameObject>();
            GameObject fxObj = PrefabUtility.InstantiatePrefab(Selection.activeGameObject) as GameObject;
            MaterialPartner[] partners = fxObj.GetComponentsInChildren<MaterialPartner>(true);
            foreach (MaterialPartner p in partners)
            {
                Renderer renderer = p.gameObject.GetComponent<Renderer>();
                if (renderer != null && renderer.sharedMaterials != null && renderer.sharedMaterials.Length > 0)
                {
                    for (int i = 0; i < renderer.sharedMaterials.Length; i++)
                    {
                        if (!collectMaterials.ContainsKey(renderer.sharedMaterials[i]))
                            collectMaterials.Add(renderer.sharedMaterials[i], renderer.gameObject);
                    }
                }
            }

            GameObject materialHookObj = Resources.Load<GameObject>(MaterialHookPath);
            GameObject InstanMaterialHookObj = GameObject.Instantiate(materialHookObj) as GameObject;

            foreach (var matInfo in collectMaterials)
            {
                Transform transform = InstanMaterialHookObj.transform.Find(matInfo.Key.name);
                if (transform == null)
                {
                    GameObject newMatObj = new GameObject() { name = matInfo.Key.name };
                    MeshRenderer mr = newMatObj.AddComponent<MeshRenderer>();
                    mr.sharedMaterials = new Material[] { matInfo.Key };
                    Debug.Log(string.Format("预制体:{0} <color=#00ff00>节点:{1}</color> <color=#ff0000>添加材质:{2}</color> 到MaterialHook", fxObj.name, matInfo.Value.name, matInfo.Key.name));
                    newMatObj.transform.SetParent(InstanMaterialHookObj.transform);
                }
                else
                {
                    Debug.Log(string.Format("预制体:{0} <color=#00ff00>节点:{1}</color> <color=#ff0000>材质:{2}</color> 已存在MaterialHook中", fxObj.name, matInfo.Value.name, matInfo.Key.name));
                }
            }

            PrefabUtility.ReplacePrefab(InstanMaterialHookObj, materialHookObj, ReplacePrefabOptions.ReplaceNameBased);

            GameObject.DestroyImmediate(InstanMaterialHookObj);
            GameObject.DestroyImmediate(fxObj);
        }

        /// <summary>
        /// 处理特效文件
        /// 1. 将MaterialPartner组件置顶(Transform下面)
        /// </summary>
        [MenuItem("PandoraTools/Fx/Process Fx", false, 1014)]
        private static void ReorderMaterialPartner()
        {
            GameObject fxObj = GameObject.Instantiate(Selection.activeGameObject) as GameObject;

            MoveMaterialPartnerToTop(fxObj);

            PrefabUtility.ReplacePrefab(fxObj, Selection.activeGameObject, ReplacePrefabOptions.ReplaceNameBased);
            GameObject.DestroyImmediate(fxObj);
        }

        private static void MoveMaterialPartnerToTop(GameObject obj)
        {
            Transform[] childTransforms = obj.GetComponentsInChildren<Transform>(true);
            foreach (Transform childTransform in childTransforms)
            {
                Component[] comps = childTransform.GetComponents<Component>();
                int moveStep = 0;
                int targetIndex = -1;
                for (int i = 0; i < comps.Length; i++)
                {
                    if (comps[i].GetType().ToString() == "com.tencent.pandora.MaterialPartner")
                    {
                        targetIndex = i;
                        break;
                    }
                    else if (comps[i].GetType().ToString() != "UnityEngine.Transform")
                        moveStep++;
                }

                if (targetIndex != -1)
                {
                    while (moveStep > 0)
                    {
                        UnityEditorInternal.ComponentUtility.MoveComponentUp(comps[targetIndex]);
                        moveStep--;
                    }
                    Debug.Log(string.Format("对象节点{0} 置顶MaterialPartner组件", comps[targetIndex].gameObject.name));
                }
            }
        }
    }
}

