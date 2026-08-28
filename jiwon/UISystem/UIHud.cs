namespace Bibimbap.UI
{
    /// <summary>
    /// 인게임 상시 표시 UI. 가장 아래 레이어에 그려지며 Screen/Popup과 독립적으로 열고 닫힌다.
    /// 예: 체력/미니맵 등의 게임플레이 HUD. 게임 씬 진입 시 열고 이탈 시 닫는다(씬 전환 시 자동으로 닫힘).
    /// </summary>
    public abstract class UIHud : UIView
    {
        public override UILayer Layer => UILayer.HUD;
    }
}
