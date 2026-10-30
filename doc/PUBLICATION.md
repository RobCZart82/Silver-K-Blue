# Public release preparation

This is the working publication checklist for Silver-K Blue. The authoritative dated task IDs are in the [Hungarian development log](../blue/Documentation/11-Fejlesztesi-naplo-es-publikalasi-terv.md).

## Prepared on this branch

- English README, Hungarian entry point, updated installation and build instructions.
- Upstream automation accepts the intended repository with either visibility, retains manual review and never merges main.
- Future packages include an English README and installation/user/skin guides.
- Release publication is explicitly restricted to manual workflow dispatch.
- Existing binary downloads are accurately identified as older test builds, without pending PR #1 fixes.

## Before changing visibility

- Review [Blue artwork provenance](../blue/Artwork/PROVENANCE.md) and record evidence for supplied/reference-derived graphics.
- Review current files, all reachable history/branches/tags, existing release archives and Actions logs/artifacts for content intended to become public. This branch does not certify that full review.
- Add authentic screenshots of Standard and Retina to the main README after rendering or capturing the actual plugin. Do not use the upstream design reference as a Blue screenshot.
- Review the existing Release descriptions, which still say “Private”; update them without changing the recorded source commit or validation history.
- Reconcile the overlapping Windows/macOS workflow edits with PR #1 when merging. Preserve both the native regressions/PR triggers and the publication changes.

## Visibility and checks

Changing repository visibility is a settings operation and does not require Actions. After the content review, public visibility can precede the blocked Windows/macOS checks. Standard hosted runner compute is free for public repositories; artifact storage is a separate allowance.

After the switch verify that the main branch ruleset is enforced, requires the actual windows and macos checks, and retains PR, conversation-resolution and up-to-date requirements. Do not bypass required checks to merge code. Re-run PR #1, resolve any actual build failures, merge only after success, then publish new packages from the accepted source.

Public visibility is not a new software release and does not change existing ZIP contents. A documentation update does not retroactively add an English guide to already uploaded downloads.

## Evidence

Record commit IDs, run URLs, package hashes, skipped tests and interactive DAW acceptance in the development log. Never label incomplete or skipped checks as passes.
