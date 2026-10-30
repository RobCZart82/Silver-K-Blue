> **Történeti pillanatfelvétel — 2026-10-25.** A lentebbi helyi archívumútvonalak, készültségi állítások és régi binárisleírások a korabeli csomagra vonatkoznak. A jelenlegi sorrend és állapot a [fejlesztési naplóban](11-Fejlesztesi-naplo-es-publikalasi-terv.md), a telepítés a [friss útmutatóban](01-Telepites-es-hasznalat.md) található. A grafikai eredet nyitott pontjai: [PROVENANCE](../Artwork/PROVENANCE.md).

# Archívum és eszközök

A 07-History/index.json 32 korábbi kimeneti mappát ír le. Az azonos tartalmú fájlok egyszer vannak tárolva a 07-History/objects mappában, SHA-256 szerint. A 1456 eredeti fájlbejegyzés és jogosultság alapján visszaállítható a könyvtárszerkezet. Az új hivatalos nevű aktuális csomag nincs önmagába archiválva.

Listázás a kibontott csomag gyökeréből:

```sh
python3 08-Tools/restore_history.py --list
```

Egy korábbi állapot visszaállítása új, még nem létező mappába:

```sh
python3 08-Tools/restore_history.py --snapshot 'Silver-K-V4-Waveform-Clean' --output '/teljes/utvonal/korabbi-waveform-clean'
```

A snapshot pontos nevét a listából másold. A visszaállító hash-t és fájlméretet ellenőriz, nem ír felül létező célmappát, és nem futtatja a fájlokat. A régi szkripteket ne indítsd el átnézés nélkül: történeti abszolút útvonalak és korábbi munkalépések vannak bennük.

Teljes csomagépség-ellenőrzés (Python 3, külön csomag nélkül):

```sh
python3 08-Tools/verify_package.py
```

A SHA256SUMS.json minden kiadási fájlt ellenőriz önmaga kivételével. A külső ZIP mellé külön SHA-256 fájl készül. A csomag fájljainak szerkesztése után az eredeti ellenőrzőösszeg természetesen eltér.

Grafikai forrásvonal célzott ellenőrzése (Pillow szükséges):

```sh
python3 08-Tools/render_retina_assets.py --check-native
```

A végleges Retina grafika újrarenderelése külön, új célmappába:

```sh
python3 08-Tools/render_retina_assets.py --output '/teljes/utvonal/retina-ujrarender'
```

Ez a mellékelt aktuális Retina skinből indul, annak JSON beállításait megőrzi, és a 256 px filmstripeket/faderkupakot rajzolja újra a mesterekből. Nem telepít, és nem írja felül a kiadási skint. A natív 64 px rajzolásnak a Standard elfogadott pixeleivel való egyezését is ellenőrzi.
