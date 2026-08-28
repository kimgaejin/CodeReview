namespace Bibimbap.UI
{
    /// <summary>
    /// 스택으로 쌓이는 UI. 나중에 열린 것이 위에 그려지고, ESC는 최상단부터 닫는다.
    /// 예: PausePopup, ConfirmPopup.
    /// 입력 차단이 필요한 팝업은 프리팹에 딤(반투명 전체 이미지) 배경을 포함시킨다.
    /// </summary>
    public abstract class UIPopup : UIView
    {
        public override UILayer Layer => UILayer.Popup;

        /// <summary>ESC로 닫을 수 있는지. 강제 선택 팝업(예: 종료 확인)은 false로 오버라이드한다.</summary>
        public virtual bool CloseOnEscape => true;
    }
}
