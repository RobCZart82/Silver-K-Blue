# Windows és macOS GitHub Actions

A két platform külön workflow-t használ:

| Workflow | Futtató | Termékek | Kötelező buildjob neve |
| --- | --- | --- | --- |
| Silver-K Blue Windows x64 | windows-2022 | VST3, Standalone | windows |
| Silver-K Blue macOS Universal 2 | macos-14 | AU, VST3, Standalone; arm64 + x86_64 | macos |

Kézi indítás: Actions → kívánt workflow → Run workflow → ellenőrzött ág kiválasztása. Teszteléskor kapcsold ki a **keep_release** bemenetet. Bekapcsolása csak sikeres build után, kézi workflow-indításkor készít Release-t.

A csomag a futás Artifacts részében érhető el, korlátozott megőrzési idővel. A tartós kiadás a Releases részben található. A forráscommitot a csomag build-info.json fájlja rögzíti, a letöltést a .zip.sha256 ellenőrzőösszeggel lehet ellenőrizni.

A kiadási job belső neve kompatibilitási okból keep-private-release maradt; a szöveges „private” jelölések megszűntek, és a működés nem igényel privát repositoryt. A PR #1 változásai adják hozzá a PR-triggerelt platform- és keretrendszer-regressziókat. A két PR összeolvasztásakor mindkét módosítás megőrzendő.

A nyilvánossá tétel nem igényel buildet. A standard futtatók nyilvános repositoryban nem fogyasztják a privát projektek havi számítási perckeretét; az artifact-tárhely külön korlát. A pontos működést nyilvánossá tétel után új futással kell ellenőrizni.

A build/ROM nélküli teszt nem DAW-os hangpróba. A nem futtatott firmware-függő ellenőrzéseket és fizikai gépes korlátokat a kiadás leírásában nevezd meg.
