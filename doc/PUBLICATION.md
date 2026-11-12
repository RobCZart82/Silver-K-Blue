# Release publication

Current published release is [1.0.0](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/blue-v1.0.0), published on 7 November 2026 from source commit `5adc19eccddc38335b7652be599766823a6d684c`. Version 1.0.1 is under development. Task IDs and acceptance gates are in the [development plan](../blue/Documentation/02-Aktualis-fejlesztesi-terv.md).

## Acceptance before publication

1. Complete the agreed version scope and update CHANGELOG.md and blue/Release-notes.md for the actual candidate. Record provenance and test limitations accurately.
2. Require successful Windows, macOS and web checks on the latest PR commit, resolved review conversations and a mergeable PR. Merge through the protected main workflow.
3. Select successful main Windows and macOS platform runs from exactly the same source commit. Record their run IDs and verify all intended fixes are included in that commit.
4. Test the actual candidate downloads with user-supplied firmware. For 1.0.1, test each selectable installer component, upgrade and removal, preservation of user data, plugin discovery and Standalone startup. Installer packaging is an open development task until these gates pass.
5. Verify package provenance, complete asset inventories, checksums and the actual signing/notarization status. Retain artwork/source provenance review in [PROVENANCE.md](../blue/Artwork/PROVENANCE.md).
6. Dispatch the main-branch Publish unified Silver-K Blue release workflow with a new numeric version, both main run IDs and the reviewed Markdown notes path.

## Publication and recovery

The publisher requires one portable ZIP, one installer and its provenance JSON from each platform run. Installer filenames, version, repository, exact source commit, platform, firmware exclusion, signature declarations and SHA-256 must match. Current Windows installers are unsigned; macOS installers are unsigned with ad hoc signed payloads and no notarization. Missing, empty, extra or mismatched installers stop publication. ZIP-only historical artifacts cannot be used with this publisher.

It assembles eight assets: two portable ZIPs, Windows EXE, macOS PKG, two installer JSON sidecars, source ZIP and SHA256SUMS.txt. The checksum list covers all seven payload/metadata files. The publisher verifies platform runs and package contents, assembles source and checksums, and creates a draft before uploading. A matching interrupted draft can be resumed. Different metadata, mismatching assets, published releases or a remote Git tag pointing at another source commit are rejected. Annotated tags are resolved to their target.

Metadata, asset inventory and the remote tag are checked again before publishing. The GitHub operations are separate requests, so concurrent editing remains a coordination risk. Avoid editing the draft during publication.

After success, inspect the published release and its downloads, update both README download entries and record the actual release commit, workflow URL and acceptance evidence in the development log. Existing published ZIPs retain their original source and contents.

## Historical context

The October 2026 visibility-preparation work and PR #1/#3 have been merged. Their former instructions to rerun or merge those PRs are complete. Historical build reports retain the evidence for their original source commits.

Record skipped firmware-dependent tests as skipped and distinguish automated checks, Rosetta testing, physical hardware and manual DAW acceptance.
