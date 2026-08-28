using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.AddressableAssets;
using UnityEngine.InputSystem;
using UnityEngine.ResourceManagement.AsyncOperations;
using Bibimbap.SceneFlow;

namespace Bibimbap.UI
{
    /// <summary>
    /// UI의 유일한 진입점. Addressables에서 UIRoot 프리팹(주소 "UIRoot")을 첫 접근 시 자동 생성한다.
    /// 씬에 미리 배치하지 않는다.
    ///
    /// - 프리팹 로드 규칙: Addressables 주소 = UIView 클래스명 (파일명·폴더는 자유)
    /// - 인스턴스는 처음 열 때 생성되어 캐싱되고, 닫으면 비활성화로 재사용된다.
    /// - 씬 전환이 시작되면 열려 있던 모든 UI를 닫는다.
    /// - ESC는 최상단 팝업을 닫는다. 팝업이 없을 때의 ESC(일시정지 열기 등)는 게임 코드의 몫이다.
    ///
    /// UI 프리팹은 작고 로컬에 있으므로 WaitForCompletion(동기 로드)을 사용한다.
    /// 프리팹이 무거워져 첫 오픈 프레임 드랍이 보이면 그때 프리로드를 추가한다.
    /// </summary>
    
    public class UIManager : MonoBehaviour
    {
        private const string RootAddress = "UIRoot";

        private static UIManager instance;

        public static UIManager Instance
        {
            get
            {
                if (instance != null) return instance;

                GameObject prefab = LoadPrefab(RootAddress, out AsyncOperationHandle<GameObject> handle);
                if (prefab == null)
                {
                    Debug.LogError($"[UI] 주소 '{RootAddress}'를 로드하지 못했다. UIRoot 프리팹의 Addressable 지정과 주소를 확인할 것.");
                    return null;
                }

                instance = Instantiate(prefab).GetComponent<UIManager>();
                instance.rootHandle = handle;
                instance.name = "UIRoot";
                return instance;
            }
        }

        [SerializeField] private RectTransform hudLayer;
        [SerializeField] private RectTransform screenLayer;
        [SerializeField] private RectTransform popupLayer;
        [SerializeField] private RectTransform toastLayer;

        private readonly Dictionary<Type, UIView> cache = new Dictionary<Type, UIView>();
        private readonly Dictionary<Type, AsyncOperationHandle<GameObject>> handles =
            new Dictionary<Type, AsyncOperationHandle<GameObject>>();
        private readonly List<UIPopup> popupStack = new List<UIPopup>();
        private UIScreen currentScreen;
        private AsyncOperationHandle<GameObject> rootHandle;

        public UIScreen CurrentScreen => currentScreen;
        public int OpenPopupCount => popupStack.Count;

        private void Awake()
        {
            if (instance != null && instance != this)
            {
                Destroy(gameObject);
                return;
            }

            instance = this;
            DontDestroyOnLoad(gameObject);
            SceneLoader.OnLoadStarted += HandleSceneLoadStarted;
        }

        private void OnDestroy()
        {
            if (instance == this) instance = null;
            SceneLoader.OnLoadStarted -= HandleSceneLoadStarted;

            foreach (AsyncOperationHandle<GameObject> handle in handles.Values)
            {
                if (handle.IsValid()) Addressables.Release(handle);
            }
            handles.Clear();

            if (rootHandle.IsValid()) Addressables.Release(rootHandle);
        }

        private void Update()
        {
            // 디버그 콘솔이 열려 있는 동안 ESC는 콘솔의 것이다.
            if (Bibimbap.Debugging.DebugSystem.ConsoleOpen) return;

            if (Keyboard.current == null || !Keyboard.current.escapeKey.wasPressedThisFrame)
                return;

            if (popupStack.Count == 0) return;

            UIPopup top = popupStack[popupStack.Count - 1];
            if (top.CloseOnEscape)
                Close(top);
        }

        /// <summary>
        /// UI를 연다. Screen이면 기존 Screen을 닫고 교체하며, Popup이면 스택 위에 쌓는다.
        /// 로드 실패 시 에러 로그 후 null.
        /// </summary>
        public T Open<T>() where T : UIView
        {
            T view = GetOrCreate<T>();
            if (view == null) return null;

            if (view.IsOpen)
                return view;

            if (view is UIScreen screen)
            {
                if (currentScreen != null)
                    CloseInternal(currentScreen);
                currentScreen = screen;
            }
            else if (view is UIPopup popup)
            {
                popupStack.Add(popup);
                popup.transform.SetAsLastSibling();
            }

            view.OpenInternal();
            return view;
        }

        public void Close(UIView view)
        {
            if (view == null || !view.IsOpen) return;
            CloseInternal(view);
        }

        public void Close<T>() where T : UIView
        {
            Close(Get<T>());
        }

        /// <summary>최상단 팝업을 닫는다. CloseOnEscape와 무관하게 닫는다.</summary>
        public bool CloseTopPopup()
        {
            if (popupStack.Count == 0) return false;

            CloseInternal(popupStack[popupStack.Count - 1]);
            return true;
        }

        public void CloseAllPopups()
        {
            for (int i = popupStack.Count - 1; i >= 0; i--)
                CloseInternal(popupStack[i]);
        }

        public void CloseAll()
        {
            CloseAllPopups();
            if (currentScreen != null)
                CloseInternal(currentScreen);
        }

        /// <summary>캐싱된 인스턴스를 반환한다. 아직 한 번도 열린 적 없으면 null.</summary>
        public T Get<T>() where T : UIView
        {
            return cache.TryGetValue(typeof(T), out UIView view) ? (T)view : null;
        }

        private void CloseInternal(UIView view)
        {
            if (view is UIScreen screen && currentScreen == screen)
                currentScreen = null;
            else if (view is UIPopup popup)
                popupStack.Remove(popup);

            view.CloseInternal();
        }

        private T GetOrCreate<T>() where T : UIView
        {
            if (cache.TryGetValue(typeof(T), out UIView cached) && cached != null)
                return (T)cached;

            string address = typeof(T).Name;
            GameObject prefab = LoadPrefab(address, out AsyncOperationHandle<GameObject> handle);
            if (prefab == null)
            {
                Debug.LogError($"[UI] 주소 '{address}'를 로드하지 못했다. 프리팹이 Addressable로 지정되고 주소가 클래스명과 같은지 확인할 것.");
                return null;
            }

            if (prefab.GetComponent<T>() == null)
            {
                Debug.LogError($"[UI] 주소 '{address}'의 프리팹에 {typeof(T).Name} 컴포넌트가 없다.");
                Addressables.Release(handle);
                return null;
            }

            T view = Instantiate(prefab, GetLayerRoot(prefab.GetComponent<T>().Layer)).GetComponent<T>();
            view.name = typeof(T).Name;
            view.gameObject.SetActive(false);

            cache[typeof(T)] = view;
            handles[typeof(T)] = handle;
            return view;
        }

        /// <summary>주소로 프리팹을 동기 로드한다. 실패하면 null을 반환하고 핸들은 해제된 상태다.</summary>
        private static GameObject LoadPrefab(string address, out AsyncOperationHandle<GameObject> handle)
        {
            handle = Addressables.LoadAssetAsync<GameObject>(address);

            GameObject prefab;
            try
            {
                prefab = handle.WaitForCompletion();
            }
            catch (Exception)
            {
                prefab = null;
            }

            if (prefab == null || handle.Status != AsyncOperationStatus.Succeeded)
            {
                if (handle.IsValid()) Addressables.Release(handle);
                handle = default;
                return null;
            }

            return prefab;
        }

        private RectTransform GetLayerRoot(UILayer layer)
        {
            switch (layer)
            {
                case UILayer.HUD: return hudLayer;
                case UILayer.Screen: return screenLayer;
                case UILayer.Popup: return popupLayer;
                case UILayer.Toast: return toastLayer;
                default: return screenLayer;
            }
        }

        private void HandleSceneLoadStarted(SceneId _)
        {
            CloseAll();
        }
    }
}
