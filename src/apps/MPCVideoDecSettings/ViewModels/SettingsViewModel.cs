using System.Collections.ObjectModel;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Microsoft.UI.Xaml;
using MPCVideoDecSettings.Services;

namespace MPCVideoDecSettings.ViewModels;

/// <summary>A single named on/off setting (one HW codec or one SW pixel format).</summary>
public partial class ToggleOption(string label, string registryName, bool isEnabled) : ObservableObject
{
    public string Label { get; } = label;
    public string RegistryName { get; } = registryName;

    [ObservableProperty]
    private bool isEnabled = isEnabled;
}

/// <summary>
/// Reads/writes the same HKCU\...\MPC Video Decoder registry values as
/// CMPCVideoDecSettingsWnd (see MPCVideoDecRegistry for the exact names),
/// and polls the filter's live status via StatusShareReader.
/// </summary>
public partial class SettingsViewModel : ObservableObject
{
    private static readonly string[] HwCodecLabels = ["MPEG2", "WMV3", "VC1", "H.264", "HEVC", "VP9", "AV1"];

    private static readonly (string Name, string Label)[] PixelFormats =
    [
        ("NV12", "NV12 (8-bit 4:2:0)"),
        ("YV12", "YV12 (8-bit 4:2:0)"),
        ("YUY2", "YUY2 (8-bit 4:2:2)"),
        ("YV16", "YV16 (8-bit 4:2:2)"),
        ("AYUV", "AYUV (8-bit 4:4:4)"),
        ("YV24", "YV24 (8-bit 4:4:4)"),
        ("P010", "P010 (10-bit 4:2:0)"),
        ("P210", "P210 (10-bit 4:2:2)"),
        ("Y410", "Y410 (10-bit 4:4:4)"),
        ("P016", "P016 (16-bit 4:2:0)"),
        ("P216", "P216 (16-bit 4:2:2)"),
        ("Y416", "Y416 (16-bit 4:4:4)"),
        ("YUV444P16", "YUV444P16 (16-bit 4:4:4, planar)"),
        ("RGB32", "RGB32"),
        ("RGB48", "RGB48"),
    ];

    // Copied 1:1 from CMPCVideoDecSettingsWnd::OnBnClickedReset() in
    // MPCVideoDecSettingsWnd.cpp, so Reset() matches the native dialog.
    private static readonly HashSet<string> DefaultDisabledPixelFormats = ["AYUV", "YUV444P16", "RGB48"];

    [ObservableProperty] private int threadNumber;
    [ObservableProperty] private int scanType;
    [ObservableProperty] private bool? arMode;
    [ObservableProperty] private bool? discardMode;

    [ObservableProperty] private int hwDecoder;
    [ObservableProperty] private string hwAdapter = "0000:0000";
    [ObservableProperty] private int dxvaCheckCompatibility;
    [ObservableProperty] private bool? disableDxvaSd;

    [ObservableProperty] private bool? swConvertToRgb;
    [ObservableProperty] private int swRgbLevels;

    [ObservableProperty] private MPCVideoDecStatus status = MPCVideoDecStatus.NotConnected;

    // Window doesn't derive from FrameworkElement, so x:Bind compiled
    // bindings on MainWindow can't resolve {StaticResource} converters.
    // Exposing Visibility directly avoids needing one for the Status panel.
    public Visibility ConnectedVisibility => Status.IsConnected ? Visibility.Visible : Visibility.Collapsed;
    public Visibility NotConnectedVisibility => Status.IsConnected ? Visibility.Collapsed : Visibility.Visible;

    partial void OnStatusChanged(MPCVideoDecStatus value)
    {
        OnPropertyChanged(nameof(ConnectedVisibility));
        OnPropertyChanged(nameof(NotConnectedVisibility));
    }

    public ObservableCollection<ToggleOption> HwCodecs { get; } = [];
    public ObservableCollection<ToggleOption> PixelFormatToggles { get; } = [];

    public SettingsViewModel()
    {
        for (int i = 0; i < HwCodecLabels.Length; i++)
        {
            HwCodecs.Add(new ToggleOption(HwCodecLabels[i], MPCVideoDecRegistry.HwCodecNames[i], isEnabled: true));
        }

        foreach (var (name, label) in PixelFormats)
        {
            PixelFormatToggles.Add(new ToggleOption(label, MPCVideoDecRegistry.SwPrefix + name, isEnabled: true));
        }

        Load();
    }

    [RelayCommand]
    private void Load()
    {
        using var key = MPCVideoDecRegistry.OpenOrCreate();

        ThreadNumber = key.GetDWord(MPCVideoDecRegistry.ThreadNumber, 0);
        ScanType = key.GetDWord(MPCVideoDecRegistry.ScanType, 0);
        ArMode = IntToTriState(key.GetDWord(MPCVideoDecRegistry.ARMode, 2));
        DiscardMode = key.GetDWord(MPCVideoDecRegistry.DiscardMode, 0) != 0;

        foreach (var codec in HwCodecs)
        {
            codec.IsEnabled = key.GetDWord(codec.RegistryName, 1) != 0;
        }

        HwDecoder = key.GetDWord(MPCVideoDecRegistry.HwDecoder, 1);
        HwAdapter = key.GetString(MPCVideoDecRegistry.HwAdapter, "0000:0000");
        DxvaCheckCompatibility = key.GetDWord(MPCVideoDecRegistry.DXVACheckCompatibility, 1);
        DisableDxvaSd = key.GetDWord(MPCVideoDecRegistry.DisableDXVA_SD, 0) != 0;

        foreach (var format in PixelFormatToggles)
        {
            bool defaultOn = !DefaultDisabledPixelFormats.Contains(StripSwPrefix(format.RegistryName));
            format.IsEnabled = key.GetDWord(format.RegistryName, defaultOn ? 1 : 0) != 0;
        }

        SwConvertToRgb = key.GetDWord(MPCVideoDecRegistry.SwConvertToRGB, 0) != 0;
        SwRgbLevels = key.GetDWord(MPCVideoDecRegistry.SwRGBLevels, 0);
    }

    [RelayCommand]
    private void Save()
    {
        using var key = MPCVideoDecRegistry.OpenOrCreate();

        key.SetValue(MPCVideoDecRegistry.ThreadNumber, ThreadNumber);
        key.SetValue(MPCVideoDecRegistry.ScanType, ScanType);
        key.SetValue(MPCVideoDecRegistry.ARMode, TriStateToInt(ArMode));
        key.SetValue(MPCVideoDecRegistry.DiscardMode, BoolToInt(DiscardMode));

        foreach (var codec in HwCodecs)
        {
            key.SetValue(codec.RegistryName, codec.IsEnabled ? 1 : 0);
        }

        key.SetValue(MPCVideoDecRegistry.HwDecoder, HwDecoder);
        key.SetValue(MPCVideoDecRegistry.HwAdapter, HwAdapter);
        key.SetValue(MPCVideoDecRegistry.DXVACheckCompatibility, DxvaCheckCompatibility);
        key.SetValue(MPCVideoDecRegistry.DisableDXVA_SD, BoolToInt(DisableDxvaSd));

        foreach (var format in PixelFormatToggles)
        {
            key.SetValue(format.RegistryName, format.IsEnabled ? 1 : 0);
        }

        key.SetValue(MPCVideoDecRegistry.SwConvertToRGB, BoolToInt(SwConvertToRgb));
        key.SetValue(MPCVideoDecRegistry.SwRGBLevels, SwRgbLevels);
    }

    /// <summary>Matches CMPCVideoDecSettingsWnd::OnBnClickedReset() exactly.</summary>
    [RelayCommand]
    private void Reset()
    {
        ThreadNumber = 0;
        ScanType = 0; // SCAN_AUTO
        ArMode = null; // indeterminate
        DiscardMode = false;

        foreach (var codec in HwCodecs)
        {
            codec.IsEnabled = true;
        }

        HwDecoder = 1; // HWDec_D3D11 (this app targets Windows 10+, always D3D11-capable)
        HwAdapter = "0000:0000";
        DxvaCheckCompatibility = 1;
        DisableDxvaSd = false;

        foreach (var format in PixelFormatToggles)
        {
            format.IsEnabled = !DefaultDisabledPixelFormats.Contains(StripSwPrefix(format.RegistryName));
        }

        SwConvertToRgb = false;
        SwRgbLevels = 0;
    }

    public void RefreshStatus() => Status = StatusShareReader.Read();

    private static string StripSwPrefix(string registryName) => registryName[MPCVideoDecRegistry.SwPrefix.Length..];

    private static bool? IntToTriState(int value) => value switch { 0 => false, 1 => true, _ => null };

    private static int TriStateToInt(bool? value) => value switch { false => 0, true => 1, null => 2 };

    private static int BoolToInt(bool? value) => (value ?? false) ? 1 : 0;
}
