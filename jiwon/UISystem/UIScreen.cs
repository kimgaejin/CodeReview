namespace Bibimbap.UI
{
    /// <summary>
    /// 전체 화면 UI. 동시에 하나만 존재하며, 새 Screen을 열면 이전 Screen은 자동으로 닫힌다.
    /// 예: TitleScreen, OptionScreen, LoadingScreen.
    /// </summary>
    public abstract class UIScreen : UIView
    {
        public override UILayer Layer => UILayer.Screen;
    }
}
