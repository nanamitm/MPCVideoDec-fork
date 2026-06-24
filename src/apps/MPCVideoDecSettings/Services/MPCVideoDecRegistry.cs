using Microsoft.Win32;

namespace MPCVideoDecSettings.Services;

/// <summary>
/// Registry key/value names, mirrored exactly from
/// src/filters/transform/MPCVideoDec/MPCVideoDec.cpp (OPT_* macros and
/// hwdec_opt_names) so this app reads/writes the same settings the native
/// filter's property page does.
/// </summary>
public static class MPCVideoDecRegistry
{
    public const string KeyPath = @"Software\MPC-BE Filters\MPC Video Decoder";

    public const string ThreadNumber = "ThreadNumber";
    public const string DiscardMode = "DiscardMode";
    public const string ScanType = "ScanType";
    public const string ARMode = "ARMode";
    public const string HwDecoder = "HwDecoder";
    public const string HwAdapter = "HwAdapter";
    public const string DXVACheckCompatibility = "DXVACheckCompatibility";
    public const string DisableDXVA_SD = "DisableDXVA_SD";
    public const string SwConvertToRGB = "SwConvertToRGB";
    public const string SwRGBLevels = "SwRGBLevels";

    public const string SwPrefix = "Sw_";

    // hwdec_opt_names[] in MPCVideoDec.cpp, indexed by MPCHwCodec.
    public static readonly string[] HwCodecNames =
    [
        "Hw_MPEG2",
        "Hw_WMV3",
        "Hw_VC1",
        "Hw_H264",
        "Hw_HEVC",
        "Hw_VP9",
        "Hw_AV1",
    ];

    // s_sw_formats[] in FormatConverter.cpp, indexed by MPCPixelFormat. The
    // registry value for each is SwPrefix + this name (e.g. "Sw_NV12").
    public static readonly string[] PixelFormatNames =
    [
        "NV12", "YV12", "YUY2", "YV16", "AYUV", "YV24",
        "P010", "P210", "Y410",
        "P016", "P216", "Y416", "YUV444P16",
        "RGB32", "RGB48",
    ];

    public static RegistryKey OpenOrCreate() => Registry.CurrentUser.CreateSubKey(KeyPath);

    public static int GetDWord(this RegistryKey key, string name, int defaultValue) =>
        key.GetValue(name) is int v ? v : defaultValue;

    public static string GetString(this RegistryKey key, string name, string defaultValue) =>
        key.GetValue(name) as string ?? defaultValue;
}
