using System.Runtime.InteropServices;
using Microsoft.UI.Xaml;
using MPCVideoDecSettings.ViewModels;
using Windows.Graphics;

namespace MPCVideoDecSettings;

public sealed partial class MainWindow : Window
{
    [DllImport("user32.dll")]
    private static extern int GetDpiForWindow(IntPtr hWnd);

    public SettingsViewModel ViewModel { get; } = new();

    private readonly DispatcherTimer _statusTimer;

    public MainWindow()
    {
        InitializeComponent();

        Title = "MPC Video Decoder Settings";

        // WinUI3's default window size is desktop-monitor-sized, not
        // settings-dialog-sized. AppWindow.Resize takes physical pixels, so
        // scale the target size by this window's own DPI instead of
        // hard-coding raw pixels (would look tiny at 100% or oversized again
        // at 200%, which is exactly the complaint this fixes).
        var hwnd = WinRT.Interop.WindowNative.GetWindowHandle(this);
        double scale = GetDpiForWindow(hwnd) / 96.0;
        AppWindow.Resize(new SizeInt32((int)(560 * scale), (int)(800 * scale)));

        // The filter has no in-proc COM connection to this app (different
        // process), so the only way to see live status is to poll the
        // shared-memory block it publishes from Transform() while decoding.
        _statusTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1) };
        _statusTimer.Tick += (_, _) => ViewModel.RefreshStatus();
        _statusTimer.Start();
        ViewModel.RefreshStatus();
    }
}
