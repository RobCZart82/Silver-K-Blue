#include "midiToSysex.h"
#include "smfSysex.h"

#include <cstring>	// memcmp

#include "baseLib/filesystem.h"

namespace synthLib
{
	bool MidiToSysex::readFile(SysexBuffer& _sysexMessages, const char* _filename)
	{
        SysexBufferList messages;
        if(!extractSysexFromFile(messages, _filename))return false;
        for(const auto& message:messages)
            _sysexMessages.insert(_sysexMessages.end(),message.begin(),message.end());
        return true;
    }

	void MidiToSysex::splitMultipleSysex(SysexBufferList& _dst, const SysexBuffer& _src, const bool _isMidiFileData/* = false*/)
	{
		if(!_isMidiFileData)
		{
			// A sysex message carries seven bit data only, no status byte may appear between its
			// $f0 and the $f7 that terminates it. Pairing every $f0 with the next $f7 without
			// checking that turns arbitrary binary into a message: preset files of other plugins
			// regularly contain such a pair by coincidence, and the caller then believes the file
			// was parsed and stops looking for the format it actually is. (BUG-10313)
			for (size_t i = 0; i < _src.size(); ++i)
			{
				if (_src[i] != 0xf0)
					continue;

				for (size_t j = i + 1; j < _src.size(); ++j)
				{
					const auto byte = _src[j];

					if (byte == 0xf7)
					{
						_dst.emplace_back().assign(_src.begin() + i, _src.begin() + j + 1);
						i = j;
						break;
					}

					if (byte & 0x80)
					{
						// not a message, resume at the offending byte, it might start one itself
						i = j - 1;
						break;
					}
				}
			}
			return;
		}

		extractSmfSysex(_src, _dst);
	}

	// A message framed twice - $f0 $f0 ... $f7 $f7 - can come from a host or wrapper that puts
	// $f0/$f7 around data which already carries them. Drop the extra byte at whichever end has one.
	void MidiToSysex::removeDuplicateFraming(SysexBuffer& _sysex)
	{
		if (_sysex.size() > 1 && _sysex[0] == 0xf0 && _sysex[1] == 0xf0)
			_sysex.erase(_sysex.begin());

		if (_sysex.size() > 1 && _sysex[_sysex.size() - 1] == 0xf7 && _sysex[_sysex.size() - 2] == 0xf7)
			_sysex.pop_back();
	}

	bool MidiToSysex::extractSysexFromFile(SysexBufferList& _messages, const std::string& _filename)
	{
		SysexBuffer data;

		if(!baseLib::filesystem::readFile(data, _filename))
			return false;

		return extractSysexFromData(_messages, data);
	}

	bool MidiToSysex::extractSysexFromData(SysexBufferList& _messages, const SysexBuffer& _data)
	{
		const bool isMidiFile = _data.size() >= 4 && std::memcmp(_data.data(), "MThd", 4) == 0;
		const auto before = _messages.size();
		if(isMidiFile && !extractSmfSysex(_data, _messages))return false;
		if(isMidiFile)return _messages.size() != before;
		splitMultipleSysex(_messages, _data, isMidiFile);
		return _messages.size() != before;
	}

}
