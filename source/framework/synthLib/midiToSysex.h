#pragma once

#include <vector>
#include <iostream>
#include <cstdint>

#include "midiTypes.h"

namespace synthLib
{
	class MidiToSysex
	{
	public:
		static bool readFile(SysexBuffer& _sysexMessages, const char* _filename);
		static void splitMultipleSysex(SysexBufferList& _dst, const SysexBuffer& _src, bool _isMidiFileData = false);
		static void removeDuplicateFraming(SysexBuffer& _sysex);
		static bool extractSysexFromFile(SysexBufferList& _messages, const std::string& _filename);
		static bool extractSysexFromData(SysexBufferList& _messages, const SysexBuffer& _data);
	};
}
