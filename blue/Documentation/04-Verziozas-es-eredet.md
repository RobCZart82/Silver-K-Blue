> **Történeti pillanatfelvétel — 2026-10-25.** A lentebbi helyi archívumútvonalak, készültségi állítások és régi binárisleírások a korabeli csomagra vonatkoznak. A jelenlegi sorrend és állapot a [fejlesztési naplóban](11-Fejlesztesi-naplo-es-publikalasi-terv.md), a telepítés a [friss útmutatóban](01-Telepites-es-hasznalat.md) található. A grafikai eredet nyitott pontjai: [PROVENANCE](../Artwork/PROVENANCE.md).

# Verziózás és eredet

| Állandó ID | Megjelenített név v1.0-nál |
|---|---|
| silver-k-blue-standard | Silver-K Blue Standard v1.0 |
| silver-k-blue-retina-4k | Silver-K Blue Retina 4K v1.0 |

Következő javításkor vagy fejlesztéskor emeljük a kiadási verziót és frissítsük a név végét, a skin.json version értékét, a RELEASE.json-t és a változásjegyzéket. Például kisebb javítás v1.0.1, új fejlesztés v1.1. Ez saját kiadási szokás, nem a Silver-K API által kikényszerített szabály. Az ID maradjon állandó; a schemaVersion a formátumé. A skinverzió és az alapplugin verziója külön dolog.

## Eredet

- Standard alap: a korábban elfogadott Silver-K V4 Setup Spacing, az első lezáró archívum 01-Standard ágából. A v1.0 címkézés csak a skin.json név/id/version metaadatait módosítja; a grafika, theme és layout érintetlen.
- Retina alap: a Silver-K R2 Navy Native Retina 256 legfrissebb elfogadott skinje. Csak név/id/version metaadat módosult.
- Bináris: a Silver-K-Tooltip-Fix-2026-10-25 legutóbbi VST3-ja változatlan bájtokkal. Az átnevezés skinmetaadat-módosítás, ezért nem kellett ismét lefordítani a hangszert.
- Kód: v0.4.4a alapforrás és helyi GUI/API javítások. Az eredeti licenc: LICENSE-Upstream.md; függőségek: 03-Source/Plugin/THIRD_PARTY.md és az upstream forrás leírásai.
- Grafika: eredeti pluginexport, felhasználói referenciák és FinaleOnOff gombok, valamint a közös munkában készült képi mesterek. A generálási szövegek a 04-Design/Masters/Prompts mappában vannak. A mesterek nem mind réteges szerkesztőformátumok; PNG-k és megőrzött munkaszkriptek állnak rendelkezésre. A forráskód licence önmagában nem igazolja minden beküldött grafika nyilvános terjesztési jogát.

A csomag nem tartalmaz ROM-ot, személyes pluginkonfigurációt vagy DAW-projektet. A megőrzött történeti dokumentumokban lehetnek korabeli helyi útvonalak; ezek tájékoztatásra és az eredet követésére szolgálnak. Publikálás előtt érdemes külön terjesztési csomagot készíteni a teljes személyes munkatörténet helyett.
