#!/usr/bin/env python3
"""Exercise the pinned JUCE VST3 processing-mode source patch."""
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
PATCH = ROOT / "cmake" / "PatchJuceVst3Prefetch.cmake"


class JuceVst3PrefetchPatchTest(unittest.TestCase):
    def apply(self, source: Path) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["cmake", f"-DSILVER_JUCE_VST3_SOURCE={source}", "-P", str(PATCH)],
            text=True,
            capture_output=True,
            check=False,
        )

    def test_rewrites_both_modes_and_is_idempotent(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "juce_audio_plugin_client_VST3.cpp"
            source.write_text(
                "setNonRealtime (newSetup.processMode == Vst::kOffline)\n"
                "setNonRealtime (data.processMode == Vst::kOffline)\n",
                encoding="utf-8",
            )

            first = self.apply(source)
            self.assertEqual(first.returncode, 0, first.stderr)
            expected = (
                "setNonRealtime (newSetup.processMode != Vst::kRealtime)\n"
                "setNonRealtime (data.processMode != Vst::kRealtime)\n"
            )
            self.assertEqual(source.read_text(encoding="utf-8"), expected)

            second = self.apply(source)
            self.assertEqual(second.returncode, 0, second.stderr)
            self.assertEqual(source.read_text(encoding="utf-8"), expected)

    def test_rejects_unrecognized_juce_source_without_writing(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "juce_audio_plugin_client_VST3.cpp"
            original = "void processAudio() {}\n"
            source.write_text(original, encoding="utf-8")

            result = self.apply(source)

            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Unrecognized JUCE VST3 processing-mode adapter", result.stderr)
            self.assertEqual(source.read_text(encoding="utf-8"), original)

    def test_patch_is_wired_for_windows_and_macos(self) -> None:
        compatibility = (ROOT / "cmake" / "JuceCompatibility.cmake").read_text(encoding="utf-8")
        self.assertIn("if(APPLE OR WIN32)", compatibility)
        self.assertIn("PatchJuceVst3Prefetch.cmake", compatibility)


if __name__ == "__main__":
    unittest.main()
