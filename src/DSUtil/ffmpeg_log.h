/*
 * (C) 2006-2025 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

extern "C" {
	#include <ExtLib/ffmpeg/libavutil/log.h>
}

#include <atomic>

// Count of "Error constructing the frame RPS" messages from the HEVC decoder.
// Checked in DecodeInternal() to flush the decoder before its internal
// reference-frame state becomes corrupt enough to crash. Always available in
// all build configurations.
inline std::atomic<int>& FFHEVCRPSErrorCount()
{
	static std::atomic<int> s_count{0};
	return s_count;
}

#define LOG_BUF_LEN 2048
inline void ff_log(void* ptr, int level, const char *fmt, va_list valist)
{
#ifdef DEBUG_OR_LOG
	if (level <= AV_LOG_VERBOSE) {
		static int print_prefix = 1;
		static char line[LOG_BUF_LEN] = {};

		av_log_format_line(ptr, level, fmt, valist, line, sizeof(line), &print_prefix);

		const size_t len = strnlen_s(line, LOG_BUF_LEN);
		if (len > 0 && line[len - 1] == '\n') {
			line[len - 1] = 0;
		}

		if (level == AV_LOG_ERROR && strstr(line, "Error constructing the frame RPS")) {
			FFHEVCRPSErrorCount().fetch_add(1, std::memory_order_relaxed);
		}

		DLog(L"FF_LOG : %hs", line);
	}
#else
	// Release: only count RPS errors to drive the HEVC flush heuristic.
	// Skip all non-error messages to avoid string-formatting overhead.
	if (level == AV_LOG_ERROR) {
		static int print_prefix = 1;
		static char line[LOG_BUF_LEN] = {};

		av_log_format_line(ptr, level, fmt, valist, line, sizeof(line), &print_prefix);

		const size_t len = strnlen_s(line, LOG_BUF_LEN);
		if (len > 0 && line[len - 1] == '\n') {
			line[len - 1] = 0;
		}

		if (strstr(line, "Error constructing the frame RPS")) {
			FFHEVCRPSErrorCount().fetch_add(1, std::memory_order_relaxed);
		}
	}
#endif
}
