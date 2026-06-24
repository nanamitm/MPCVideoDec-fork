/*
 * (C) 2026 see Authors.txt
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

#include <windows.h>

// Exposes the same status fields as IMPCVideoDecFilter::GetInformation()
// to out-of-process tools (e.g. a companion settings UI) that have no
// in-proc COM connection to a running filter instance.
//
// This is a single global slot: if more than one filter instance is
// active at once (e.g. two players running concurrently), the most
// recently published instance wins. There is no general way to pick
// "the" instance a reader cares about across arbitrary host processes,
// so this is accepted as a known limitation rather than solved here.
struct MPCVideoDecStatusBlock
{
	LONG  Sequence;  // seqlock: odd while a writer is updating, even when stable
	DWORD ProcessId; // 0 = no active instance published
	WCHAR InputFormat[128];
	WCHAR FrameSize[64];
	WCHAR OutputFormat[128];
	WCHAR ActiveDecoder[128];
	WCHAR GraphicsAdapter[128];
};

inline constexpr wchar_t MPCVideoDecStatusShareName[] = L"Local\\MPCVideoDecStatus";

// RAII publisher. Readers map MPCVideoDecStatusShareName and use the same
// seqlock protocol to get a torn-read-free snapshot without a cross-process
// mutex, since updates are only a handful of small string fields a few
// times a second at most.
class CMPCVideoDecStatusPublisher
{
	HANDLE m_hMapping = nullptr;
	MPCVideoDecStatusBlock* m_pBlock = nullptr;

	bool EnsureOpen()
	{
		if (m_pBlock) {
			return true;
		}

		m_hMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
										 0, sizeof(MPCVideoDecStatusBlock), MPCVideoDecStatusShareName);
		if (!m_hMapping) {
			return false;
		}

		m_pBlock = (MPCVideoDecStatusBlock*)MapViewOfFile(m_hMapping, FILE_MAP_WRITE, 0, 0, sizeof(MPCVideoDecStatusBlock));
		if (!m_pBlock) {
			CloseHandle(m_hMapping);
			m_hMapping = nullptr;
			return false;
		}

		return true;
	}

public:
	~CMPCVideoDecStatusPublisher()
	{
		// Mark the slot disconnected before unmapping so a reader that
		// already has the section open (which keeps it alive past this
		// process exiting) doesn't keep showing stale "still playing" data.
		Clear();

		if (m_pBlock) {
			UnmapViewOfFile(m_pBlock);
		}
		if (m_hMapping) {
			CloseHandle(m_hMapping);
		}
	}

	void Publish(LPCWSTR inputFormat, LPCWSTR frameSize, LPCWSTR outputFormat, LPCWSTR activeDecoder, LPCWSTR graphicsAdapter)
	{
		if (!EnsureOpen()) {
			return;
		}

		InterlockedIncrement(&m_pBlock->Sequence); // -> odd: write in progress
		m_pBlock->ProcessId = GetCurrentProcessId();
		wcsncpy_s(m_pBlock->InputFormat, inputFormat, _TRUNCATE);
		wcsncpy_s(m_pBlock->FrameSize, frameSize, _TRUNCATE);
		wcsncpy_s(m_pBlock->OutputFormat, outputFormat, _TRUNCATE);
		wcsncpy_s(m_pBlock->ActiveDecoder, activeDecoder, _TRUNCATE);
		wcsncpy_s(m_pBlock->GraphicsAdapter, graphicsAdapter, _TRUNCATE);
		InterlockedIncrement(&m_pBlock->Sequence); // -> even: stable
	}

	void Clear()
	{
		if (!m_pBlock) {
			return;
		}

		InterlockedIncrement(&m_pBlock->Sequence);
		m_pBlock->ProcessId = 0;
		InterlockedIncrement(&m_pBlock->Sequence);
	}
};
