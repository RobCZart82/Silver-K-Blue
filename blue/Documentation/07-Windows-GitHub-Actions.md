# Windows build a privát GitHub repositoryban

A repository a fordítás receptjét is megőrzi. Újratelepítés után egy böngésző és a GitHub-fiókod elég a Windows-build indításához; nem kell a saját Macre Windows fejlesztőkörnyezet.

## Indítás

1. Nyisd meg a https://github.com/RobCZart82/Silver-K-Blue/actions oldalt.
2. Válaszd a **Silver-K Blue Windows x64** munkafolyamatot.
3. **Run workflow**, majd a kívánt ág (általában main), ismét **Run workflow**. A **Save successful package as a durable private Release** kapcsoló alapból bekapcsolt; hagyd így a tartós GitHub-mentéshez.
4. A sikeres futás oldalán az **Artifacts** részből töltsd le a **Silver-K-Blue-Windows-x64-…** csomagot. Ebben egy ZIP és annak SHA-256 ellenőrzőösszege van.

A csomag Windows x64 VST3-at, önálló EXE-t, mindkét beépített Blue skint és az opcionális külső forrásokat, telepítési leírást és pontos forráscommitot tartalmaz. Firmware nincs feltöltve. A beépített skinekhez nem kell Skins mappát telepíteni.

## Megőrzés és hozzáférés

A forrás, a skinek és a workflow a repositoryban maradnak. A kész Actions-letöltés **30 nap után lejár**; a diagnosztika 14 napig marad. A bekapcsolt Release-mentés siker esetén külön privát Release mellékleteként is megőrzi a ZIP-et és az ellenőrzőösszegét; ezeket nem az Actions 30 napos lejárata kezeli. A repository **Releases** oldalán később is letölthetők, amíg a kiadást vagy a repositoryt nem törlöd. Kikapcsolt Release-mentésnél töltsd le saját tartós mentéshez. Lejárat után ugyanabból a forráscommitból új build indítható, amíg a fordítók és a külső függőségek elérhetők.

A repository továbbra is privát. A letöltésekhez GitHub-bejelentkezés és repository-hozzáférés kell. A workflow kézi indítású: nem indul el minden szerkesztés/feltöltés után. Egy időben ugyanazon ágon egy build fut. A privát futások a fiók GitHub Actions keretét használják; nem változtattunk számlázási vagy költési beállítást.

## Technikai recept

- Windows Server 2022 GitHub-hosted runner, Visual Studio x64, CMake Release.
- Rögzített upstream függőségrevíziók: cmake/Dependencies.cmake.
- Célok: silverK_VST3, silverK_Standalone; öt ROM nélküli teszt: konverzió, host clock, latency reentry, editor, Performance editor.
- Csomagolás: tools/package_blue_windows.py; mindkét bináris PE x64 géptípusát és a ZIP épségét ellenőrzi.
- Workflow: .github/workflows/windows-blue.yml. A régi upstream workflow-k csak minták a blue/Upstream-Workflow-Examples mappában.

A Windows DAW-ban végzett hang- és működési próba a sikeres build után is szükséges. A fordítás sikere nem jelenti, hogy minden hangszín/host ellenőrzött. Az EXE és a VST3 nem kapott gyártói kódaláírást.

GitHub útmutatók: https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow és https://docs.github.com/en/actions/how-tos/manage-workflow-runs/download-workflow-artifacts .

## Első beépített kiadás

Az elkészült csomagok és a tényleges teszt-/SKIP státuszok: [10-Elso-beepitett-kiadas.md](10-Elso-beepitett-kiadas.md). A számlázási/keretkorlátozás miatt a Release-utófeladat megállt; a csomagokat külön privát Release-be mentettük.
