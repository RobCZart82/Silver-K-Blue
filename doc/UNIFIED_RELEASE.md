# Unified Blue release

Use the manual **Publish unified Silver-K Blue release** workflow on main after its platform checks pass. Supply a new numeric version and the Windows/macOS Actions run IDs. Both must be successful main builds from the same commit. Platform build workflows upload artifacts only.

The publisher validates workflow identity, source commit, package provenance and every recorded file checksum. It publishes one Release containing Windows, macOS Universal 2, source ZIP and SHA256SUMS.txt. It never deletes historical releases or overwrites existing versions. An interrupted upload leaves a draft for manual inspection. GitHub also supplies automatic source archives; the explicitly uploaded Source.zip is the verified build-resource snapshot. External dependencies remain fetched at pinned revisions.

After publication update README download links, the release manifest and development log with the exact commit and run URLs. Do not describe skipped firmware tests as passes. Old platform Release instructions in historical logs are superseded by this process.
