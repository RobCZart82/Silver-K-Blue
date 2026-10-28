# Fordítás és átadás

A 03-Source/Plugin mappa tartalmazza az upstream v0.4.4a forrásarchívumát, a négy módosított produkciós fájlt, a teljes helyi patch-et és a korábbi minimális tooltip-patch-et. A 03-Source/Tests tesztjei a fejlesztés ellenőrzését őrzik; a kiadási extra tesztág nem változtat produkciós működést.

A teljes újraépítéshez a **full-v0.4.4a-to-tooltip-fix.patch** alkalmazandó a tiszta, mellékelt alapforrásra. A **tooltip-only-over-native-fonts.patch** csak a korábbi Native Fonts helyi alapra való, és nem alkalmazandó a teljes patch után még egyszer. Részletes parancsok: 03-Source/Plugin/BUILD.md; platformfüggőségek: 03-Source/Upstream-BUILD.md.

Az első CMake-konfigurálás függőségeket tölt le, ezért a forrásátadás nem teljes offline fejlesztőkörnyezet. A kiadási csomag, a kész plugin és a skinek telepítéséhez nincs újrafordítás vagy hálózati függőségletöltés.

A macOS bináris Release, arm64 + x86_64, macOS 12.0 minimumcéllal készült. A kiadási ellenőrzés a meglévő legutolsó bináris változatlanságát ellenőrzi. A v1.0 névátvezetés csak JSON metaadat, nem hoz létre új hangmotor-buildet.

A fordítást és a teszteket elkülönített adatkönyvtárral végezd; a SILVER_K_DATA változó erre szolgál. A firmware-t igénylő régebbi tesztek saját, jogszerűen rendelkezésre álló ROM-útvonalakat várnak, ezek nincsenek a csomagban. A végleges név- és tooltipellenőrzések firmware nélkül is futtathatók.
