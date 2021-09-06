using UnityEngine;
using UnityEngine.UI;
using UnityEditor;
using System.Collections;

namespace com.tencent.pandora.tools
{
    public class WaterfallScrollViewCreator
    {
        private static Vector2 CENTER = new Vector2(0.5f, 0.5f);
        private static Vector2 TOP_LEFT = new Vector2(0, 1);

        [MenuItem("PandoraTools/组件生成器/瀑布流")]
        public static void Create()
        {
            GameObject scrollView = CreateGameObject("WaterfallScrollView", CENTER, CENTER, CENTER, null);
            GameObject pool = CreateGameObject("Container_pool", CENTER, CENTER, CENTER, scrollView);
            GameObject template = CreateGameObject("itemTemplate", TOP_LEFT, TOP_LEFT, TOP_LEFT, scrollView);
            GameObject viewpot = CreateGameObject("Viewport", new Vector2(0, 0), new Vector2(1, 1), TOP_LEFT, scrollView);
            (viewpot.transform as RectTransform).sizeDelta = Vector2.zero;
            GameObject content = CreateGameObject("Content", TOP_LEFT, TOP_LEFT, TOP_LEFT, viewpot);
            ScrollRect scrollRect = scrollView.AddComponent<ScrollRect>();
            scrollRect.vertical = true;
            scrollRect.horizontal = false;
            scrollRect.viewport = viewpot.transform as RectTransform;
            scrollRect.content = content.transform as RectTransform;
            WaterfallScrollViewHelper helper = scrollView.AddComponent<WaterfallScrollViewHelper>();
            helper.itemPool = pool;
            helper.itemTemplate = template;
            viewpot.AddComponent<Mask>();
            viewpot.AddComponent<Image>();
        }

        private static GameObject CreateGameObject(string name, Vector2 anchorMin, Vector2 anchorMax, Vector2 pivot, GameObject parent)
        {
            GameObject go = new GameObject(name);
            RectTransform trans = go.AddComponent<RectTransform>();
            if (parent != null)
            {
                trans.SetParent(parent.transform);
            }
            trans.anchoredPosition = Vector2.zero;
            trans.anchorMin = anchorMin;
            trans.anchorMax = anchorMax;
            trans.pivot = pivot;
            return go;
        }
    }
}

