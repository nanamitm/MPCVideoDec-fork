#pragma once

// Registry key/value names mirrored exactly from
// src/filters/transform/MPCVideoDec/MPCVideoDec.cpp (OPT_* macros and
// hwdec_opt_names) and src/filters/transform/MPCVideoDec/FormatConverter.cpp
// (s_sw_formats), so this app reads/writes the same settings the native
// filter's property page does. Kept in sync by hand with the C# port at
// src/apps/MPCVideoDecSettings/Services/MPCVideoDecRegistry.cs.

#include <windows.h>
#include <array>
#include <string>

namespace MPCVideoDecRegistry {

inline constexpr wchar_t KeyPath[] = L"Software\\MPC-BE Filters\\MPC Video Decoder";

inline constexpr wchar_t ThreadNumber[] = L"ThreadNumber";
inline constexpr wchar_t DiscardMode[] = L"DiscardMode";
inline constexpr wchar_t ScanType[] = L"ScanType";
inline constexpr wchar_t ARMode[] = L"ARMode";
inline constexpr wchar_t HwDecoder[] = L"HwDecoder";
inline constexpr wchar_t HwAdapter[] = L"HwAdapter";
inline constexpr wchar_t DXVACheckCompatibility[] = L"DXVACheckCompatibility";
inline constexpr wchar_t DisableDXVA_SD[] = L"DisableDXVA_SD";
inline constexpr wchar_t SwConvertToRGB[] = L"SwConvertToRGB";
inline constexpr wchar_t SwRGBLevels[] = L"SwRGBLevels";
inline constexpr wchar_t SwPrefix[] = L"Sw_";

// hwdec_opt_names[] in MPCVideoDec.cpp, indexed by MPCHwCodec.
inline constexpr std::array<const wchar_t*, 7> HwCodecNames = {
	L"Hw_MPEG2", L"Hw_WMV3", L"Hw_VC1", L"Hw_H264", L"Hw_HEVC", L"Hw_VP9", L"Hw_AV1",
};
inline constexpr std::array<const wchar_t*, 7> HwCodecLabels = {
	L"MPEG2", L"WMV3", L"VC1", L"H.264", L"HEVC", L"VP9", L"AV1",
};

// s_sw_formats[] in FormatConverter.cpp, indexed by MPCPixelFormat. The
// registry value for each is SwPrefix + this name (e.g. "Sw_NV12").
inline constexpr std::array<const wchar_t*, 15> PixelFormatNames = {
	L"NV12", L"YV12", L"YUY2", L"YV16", L"AYUV", L"YV24",
	L"P010", L"P210", L"Y410",
	L"P016", L"P216", L"Y416", L"YUV444P16",
	L"RGB32", L"RGB48",
};
inline constexpr std::array<const wchar_t*, 15> PixelFormatLabels = {
	L"NV12 (8-bit 4:2:0)", L"YV12 (8-bit 4:2:0)", L"YUY2 (8-bit 4:2:2)", L"YV16 (8-bit 4:2:2)",
	L"AYUV (8-bit 4:4:4)", L"YV24 (8-bit 4:4:4)",
	L"P010 (10-bit 4:2:0)", L"P210 (10-bit 4:2:2)", L"Y410 (10-bit 4:4:4)",
	L"P016 (16-bit 4:2:0)", L"P216 (16-bit 4:2:2)", L"Y416 (16-bit 4:4:4)", L"YUV444P16 (16-bit 4:4:4, planar)",
	L"RGB32", L"RGB48",
};

// Copied 1:1 from CMPCVideoDecSettingsWnd::OnBnClickedReset() in
// MPCVideoDecSettingsWnd.cpp, so Reset() matches the native dialog.
inline bool IsDefaultDisabledPixelFormat(const wchar_t* name)
{
	return wcscmp(name, L"AYUV") == 0 || wcscmp(name, L"YUV444P16") == 0 || wcscmp(name, L"RGB48") == 0;
}

inline HKEY OpenOrCreate()
{
	HKEY hKey = nullptr;
	RegCreateKeyExW(HKEY_CURRENT_USER, KeyPath, 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &hKey, nullptr);
	return hKey;
}

inline DWORD GetDWord(HKEY key, const wchar_t* name, DWORD defaultValue)
{
	DWORD value = defaultValue;
	DWORD size = sizeof(value);
	DWORD type = 0;
	if (!key || RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(&value), &size) != ERROR_SUCCESS || type != REG_DWORD) {
		return defaultValue;
	}
	return value;
}

inline void SetDWord(HKEY key, const wchar_t* name, DWORD value)
{
	if (key) {
		RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
	}
}

inline std::wstring GetString(HKEY key, const wchar_t* name, const std::wstring& defaultValue)
{
	wchar_t buffer[256] = {};
	DWORD size = sizeof(buffer);
	DWORD type = 0;
	if (!key || RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size) != ERROR_SUCCESS || type != REG_SZ) {
		return defaultValue;
	}
	return buffer;
}

inline void SetString(HKEY key, const wchar_t* name, const std::wstring& value)
{
	if (key) {
		RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
	}
}

inline std::wstring SwName(const wchar_t* pixelFormatName)
{
	return std::wstring(SwPrefix) + pixelFormatName;
}

} // namespace MPCVideoDecRegistry
