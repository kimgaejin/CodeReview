using UnityEngine;

namespace Bibimbap.UI
{
    /// <summary>
    /// UI가 그려지는 계층. UIRoot 안의 레이어 순서(형제 순서)가 곧 그리기 순서다.
    /// </summary>
    public enum UILayer
    {
        HUD = 0,      // 인게임 상시 표시
        Screen = 1,   // 전체 화면. 동시에 하나만 열린다
        Popup = 2,    // 스택으로 쌓인다
        Toast = 3,    // 알림. 입력을 막지 않는다
    }

    /// <summary>
    /// 모든 UI 프리팹의 베이스. 열고 닫는 것은 UIManager만 한다.
    /// 프리팹은 Resources/UI/ 아래에 클래스 이름과 같은 이름으로 둔다. (예: PausePopup.prefab)
    ///
    /// 등장/퇴장 애니메이션이 필요해지면 OnOpened/OnClosed에서 DOTween을 쓰되,
    /// UI는 일시정지 중에도 움직여야 하므로 SetUpdate(true)로 unscaled time을 사용할 것.
    /// </summary>
    
    public abstract class UIView : MonoBehaviour
    {
        public abstract UILayer Layer { get; }

        public bool IsOpen { get; private set; }

        internal void OpenInternal()
        {
            gameObject.SetActive(true);
            IsOpen = true;
            OnOpened();
        }

        internal void CloseInternal()
        {
            OnClosed();
            IsOpen = false;
            gameObject.SetActive(false);
        }

        /// <summary>열린 직후 호출된다. 값 갱신, 포커스 지정 등에 사용한다.</summary>
        protected virtual void OnOpened() { }

        /// <summary>닫히기 직전 호출된다. 편집 중이던 상태 정리 등에 사용한다.</summary>
        protected virtual void OnClosed() { }
    }
}
