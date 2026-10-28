# Telepítés és használat

## Skin

Az **01-Skins** két közvetlen almappáját másold a plugin által a Setup → Skins folder gombbal megnyitott mappába. A szokásos hely ezen a gépen: `/Users/robert/Documents/Silver-K/Skins/`, más gépen a plugin által megnyitott hely az irányadó.

A skin.json fájl `name` mezője adja a lista nevét; nem pusztán a mappanév. A kiadás mindkettőt egységesíti. A `schemaVersion: 1` a fájlformátum verziója, ezt ne emeld a skin kiadási verziójával együtt. A `version: "1.0"` kiadási metaadat, a megjelenítéshez a teljes név is tartalmazza a verziót.

Korábbi skinek maradhatnak külön biztonsági másolatban, de a két véglegesből egyszerre egy-egy példányt tarts az aktív Skins mappában. A plugin beépített skinjei továbbra is láthatók lehetnek; ez a csomag két külső skint ad hozzá. Ha egy régi projekt korábbi csomagra mutat, válaszd ki a végleges nevet és mentsd újra a projektet/beállítást.

## VST3 macOS-on

A mellékelt helyi build Apple Silicon és Intel kódot tartalmaz, minimumcélja macOS 12.0. Ad hoc aláírású helyi build, nem közjegyzett gyártói telepítő. A csomag nem tartalmaz Windows-, Linux- vagy AU-binárist.

1. Ments és zárj be minden Silver-K-t használó hostot.
2. A jelenlegi Silver-K.vst3 példányt mentsd el a VST3 keresési útvonalakon kívülre.
3. A **02-Plugin/macOS-Universal2/Silver-K.vst3** teljes bundle-t másold a használt VST3 helyre. A felhasználói hely általában `~/Library/Audio/Plug-Ins/VST3/`, a közös hely `/Library/Audio/Plug-Ins/VST3/`. Azt használd, amelyikből a hostod eddig is töltötte; ne hagyd a régi azonosítójú példányt egy másik beolvasott helyen.
4. Indítsd el a hostot, szükség esetén frissítsd a pluginlistát, majd a Setup oldalon válaszd ki a végleges skint.

A plugin neve és azonosítója továbbra is Silver-K. A két új név a skinválasztó neve, nem két külön hangszer.

## Mire számíts?

- Standard: a korábban elfogadott klasszikus mezőket és natív LCD-szöveget őrzi; kompatibilis az eredeti v0.4.4a-val.
- Retina: nagyobb forgatógomb- és faderképek; sötét R2 LCD-hátterű mezők eredeti betűvel; 4×24 karakteres fő pontmátrixos LCD, sortöréssel. Performance presetnevek és a mezők betűi is natívak a legutóbbi döntés szerint.
- A forgatógomb kék közepe és értékjelző nyelve együtt fordul. A 21 LED pozíciója rögzített, a felgyulladás értékfüggő. A középen hófehér jelölést kapó LED-verzió maradt meg; nincs új fényudvar-változat hozzáadva.
- A K-Station firmware nem támogat négyrészes KS Performance működést. A helyi build az ott nem használható képes vezérlőket is halványítja. Ez kijelzésjavítás, nem új négyrészes hangképesség.

A firmware-t és a saját bankokat a meglévő Silver-K adatkönyvtárból használd. Visszaállításhoz tedd vissza a mentett VST3-at és válaszd a Standard skint; a Retina magas stripjeit a gyári v0.4.4a nem tudja betölteni.
