# DS Style Kernel Audit — Standalone Handoff

This file is for a fresh Codex session with no conversation history. Read this file first, then read `CODEX_KERNEL_AUDIT_PROGRESS.md`, and reconcile both against the live Git state before editing. This handoff summarizes the current state; the progress file is the detailed evidence ledger.

## Cold-start brief — authoritative current state (2026-10-14)

This section is written for an auditor who knows nothing about this repository or prior work. It supersedes stale historical checkpoints later in this file where they disagree.

- Project: Omega DE DS Style Multisystem Kernel, an embedded GBA-format kernel for the EZ-FLASH OMEGA Definitive Edition. Main firmware is C with ARM assembly; FatFs is under `source/ff15`; the browser style installer is under `docs/manager/js`.
- Workspace: `/workspaces/omega-de-ds-style-multisystem-kernel`.
- Branch: `feature/mirrored-artwork`, tracking `origin/feature/mirrored-artwork`.
- Branch: `feature/mirrored-artwork`, tracking `origin/feature/mirrored-artwork`. The latest firmware/source commit before this ledger-preservation update is `daf4f45f1c730f261084ef9e87b9bef8c0da83f5` (`Update kernel splash artwork`); inspect `git log` for the ledger commit that follows it. Firmware audit/release commit: `a72e5354acb26c5491e764a43dc2b5392e7c02ed` (`Audit and harden kernel error paths`).
- Current tree at handoff update: production source and assets are committed/pushed; the two audit ledgers are now also committed/pushed by explicit user request so they survive Codespace deletion. Recheck status before every edit; preserve any new user work.
- Both audit ledgers are tracked on `feature/mirrored-artwork`. Keep them updated with verified checkpoints; exclude them from release binaries and release notes.
- Release: GitHub tag `v7.4.1` points to firmware audit commit `a72e535…`. The release has only `ezkernelnew.bin` (2,089,664 bytes). On 2026-10-14 the user-supplied splash was rebuilt into the kernel and this asset was replaced; downloaded release asset SHA-256 matched local build `9db6fa539c7e812e653bb4b4f9fbd60a48f0c938a63849b81a83e7c364944be7`. The corresponding splash source/header were committed as `daf4f45` and pushed. Release notes contain major changes/fixes and AI attribution. The tag itself remains on the audit commit; do not recreate or retarget this release unless a later, justified firmware change requires a new publication.
- Splash update (2026-10-14): selected `files/SPLASH.png` (240x160 indexed-color PNG, 4-bit) as the source; it is the user-provided warning splash and matched the BMP alternative. Replaced `images/SPLASH.png`; regenerated `images/SPLASH.h` with devkitPro Grit 0.9.2 using the repository pipeline flags (`-gu8 -gb -gB16 -ftc`) and script-equivalent symbol replacements. Header is 240x160@16 with 76,800 bytes for the kernel's `u16*` drawing path. `make` passed, generated release asset matches the local build by SHA-256, and the requested `files/` input folder was removed after verification. Root `.gba` and `.elf` intermediates were removed; keep `ezkernelnew.bin` and `build/`.
- Current audit state: most source/UI/assembly/docs review and fixes are recorded below; F-011 settings-write recovery remains unresolved pending a safely testable design. F-002, F-006, and F-009 are deferred low-priority findings. No physical cartridge/hardware validation has been performed.
- Cartridge release asset `ezkernelnew.bin` is the retained kernel artifact. The user has authorized deleting the root `omega-de-ds-style-multisystem-kernel.elf` and `omega-de-ds-style-multisystem-kernel.gba` after the `.bin` has been posted to the release; those two files are disposable build intermediates and have now been removed. For future builds, verify the `.bin` exists and matches the successful `.gba` output before removing the root `.elf`/`.gba`. Preserve `build/` and its map/object/dependency files; do not run `make clean` solely for this cleanup.

### Start from total ignorance

1. Read this file, then `CODEX_KERNEL_AUDIT_PROGRESS.md`. Treat all old dated notes as evidence/history, not as the live repository state.
2. Inspect repository instructions from scratch: `pwd`, `git status --short --branch --untracked-files=all`, `git rev-parse HEAD`, upstream/tracking, `rg --files -g AGENTS.md`, README, Makefile, `build.bat`, `.devcontainer/`, `scripts/`, `source/`, tests, workflows, docs, and `git log`. Do not assume the branch, release, toolchain, permissions, or files match this checkpoint. Preserve unknown edits and never clean/reset/rebase/switch without explicit need and authorization.
3. Map the firmware before auditing: entrypoint/startup; launcher state and views; filesystem/FatFs; SD/MMC/USB backends; save/settings/RTS persistence; PSRAM/NOR/SRAM; cheat parser; render/artwork; localization/audio; C and assembly patch/IRQ/reset paths; build/assets; manager JS/tests/CI; user documentation. Trace callers and failure paths, not just individual functions.
4. For each issue record a stable finding ID, severity and confidence, exact file/line evidence, triggering input/state, consequence, relevant call path, fix rationale, and validation limits. Distinguish confirmed defects from analyzer noise, probable issues, and architectural limitations. Check bounds, integer overflow/truncation, initialization, lifetimes, partial I/O, close/flush status, cleanup, retry/idempotency, and state recovery. Review changed code adversarially after the authoring pass.
5. Do not claim static review proves hardware behavior. Never flash or issue hardware writes in a host audit. Add/run tests only when requested or already included in the authorized audit/verification workflow; do not invent destructive test setups.

### Recreate the Codespaces/devkitPro environment

The supported Codespaces setup is the checked-in devcontainer, not a separate installer. `scripts/install-devkitpro.sh` was removed because no workflow referenced it, the direct bootstrap returned HTTP 403, and the devcontainer now uses the official image.

1. Open the repository as a GitHub Codespace using its `.devcontainer/devcontainer.json`; it builds `.devcontainer/Dockerfile`, based on `devkitpro/devkitarm:20260610`, and sets `DEVKITPRO=/opt/devkitpro`, `DEVKITARM=/opt/devkitpro/devkitARM`.
2. If changing the devcontainer on an existing Codespace, rebuild/reopen it so the image is applied. The first invocation of `scripts/rebuild-and-verify-audit.sh` can request `gh codespace rebuild` when `DEVKITARM` is absent; this requires an authenticated `gh` session and `CODESPACE_NAME`. After reconnect, open a new terminal and run it again. Do not use this trigger casually if the working tree contains uncommitted work; verify Codespaces rebuild-preservation behavior/current docs first.
3. Confirm `printf '%s\n' "$DEVKITPRO" "$DEVKITARM"`, `"$DEVKITARM/bin/arm-none-eabi-gcc" --version`, and that `"$DEVKITARM/gba_rules"` exists. Do not print tokens or unrelated environment secrets.
4. Run `make` (not `make clean`). The Linux/Codespaces output is `omega-de-ds-style-multisystem-kernel.gba`. The Windows `build.bat` workflow copies its successful output to `ezkernelnew.bin`, which is the cartridge release filename. The two files are byte-identical in the verified v7.4.1 package.
5. Run `scripts/rebuild-and-verify-audit.sh` to build without cleanup and run the existing browser installer scenarios, manager JS syntax checks, whitespace check, and optional Cppcheck. It does not stage, commit, push, or clean. Also run narrowly targeted analysis for changed modules and explain any warnings. Theme generation is Windows/PowerShell-oriented and was not run in the previous audit.
6. The previous Codespace had devkitARM 16.1.0, GNU Make 4.3, Node 24.21.0, cppcheck 2.13.0, `gh` 2.23.0, git-lfs 3.3.0, and Poppler installed. These package installs may not persist across rebuilds. The devcontainer image itself is the durable devkitPro setup; do not resurrect the removed installer.

### Audit continuation goal

Resume with F-011: decide whether a host-side FatFs media/fault-injection harness can safely exercise unified settings replacement and startup recovery. The production serializer opens `SETTINGS.TXT` with `FA_CREATE_ALWAYS`; checking `f_printf`/`f_close` alone does not protect the old file from truncation. FatFs rename consists of multiple directory operations, so a backup/temp design needs startup recovery. Do not modify this persistence flow until recovery semantics and fault-path verification are concrete. If the harness is infeasible, document exactly why and bound the audit; otherwise implement the smallest safe recovery design and test success, short-write, full-media, close/flush failure, interrupted rename, and recovery cases.

After F-011, finish a source inventory-based review of any modules not evidenced in the progress ledger, reconcile findings F-001–F-017 and deferred F-002/F-006/F-009, update both ledgers, run the targeted verification without cleanup, and leave the release untouched unless new justified firmware changes warrant a follow-up. These audit ledgers are tracked on the branch by user request and should be committed/pushed when updated; exclude them from release assets.

## Objective and operating rules

Complete the comprehensive, evidence-based engineering audit of the EZ-FLASH OMEGA Definitive Edition DS Style kernel on the existing `feature/mirrored-artwork` branch. Review the implementation, rendering and artwork paths, GBA/ARM assembly, memory and filesystem boundaries, settings persistence, build and asset workflows, tests, comments, documentation, and upstream integration. Implement justified fixes, verify them, perform an adversarial diff review, and leave a release-ready result.

Preserve the current branch and any user work. Do not reset, clean, rebase, merge, or switch branches. Do not flash hardware. Do not claim hardware behavior was tested. Do not broadly rewrite upstream FatFs. Keep the audit ledgers tracked and current; they are not firmware release assets. Generated build outputs are ignored and must not be deleted merely to tidy the tree.

**Publication status:** the user authorized the audited firmware commit/push and v7.4.1 release. That release is already live and contains `ezkernelnew.bin`; it must not be recreated for documentation-only work. Audit ledgers are committed to the development branch per later user instruction, but must remain out of firmware assets/releases. Any future release must be tied to a justified source change and current authorization/tool policy, not inferred from this completed release.

## Original audit baseline (historical; see cold-start brief for live state)

- Workspace: `/workspaces/omega-de-ds-style-multisystem-kernel`
- Branch: `feature/mirrored-artwork`, tracking `origin/feature/mirrored-artwork`
- Starting HEAD before the audit work: `63ee36690e2d81280ff528fb1a9471a63843a999`, also the local `Multisystem` tag and origin branch tip at that time.
- No staged changes. No pre-audit user modifications were present at audit start.
- At the release commit, production files were committed; the two audit ledgers were local at that historical point. They were later committed to the development branch by explicit user request. A subsequent cleanup removes the unreferenced `scripts/install-devkitpro.sh`; the devcontainer uses the official devkitARM image directly.
- Historical note: the initial build created ignored `build/` files, `build/omega-de-ds-style-multisystem-kernel.map`, and root `.elf`/`.gba`; the user later authorized removing the root `.elf` and `.gba` after release packaging. Recheck artifacts on resume and retain `ezkernelnew.bin`.

## Latest completion and publication

- Production commit: `a72e5354acb26c5491e764a43dc2b5392e7c02ed` (`Audit and harden kernel error paths`), pushed to `origin/feature/mirrored-artwork`.
- Follow-up cleanup commit: `beaf155` (`Remove redundant devkitPro installer`), pushed to the same branch. This only deletes the unused installer; it does not alter the released firmware or ROM asset.
- GitHub Release `v7.4.1` was created against that exact commit: https://github.com/voluptuary/omega-de-ds-style-multisystem-kernel/releases/tag/v7.4.1
- Verified release metadata: tag `v7.4.1`, target commit `a72e5354acb26c5491e764a43dc2b5392e7c02ed`, sole asset `ezkernelnew.bin` (2,089,664 bytes, uploaded). Release notes list major changes/fixes and end with AI attribution. The `.bin` is byte-identical to the Makefile `.gba` output (SHA-256 `6eeac9c77d3b2406ab6b0f994735e613e04e16974ba562b3e9fba1176a43a2e3`); `build.bat` packages the same output under this cartridge filename.
- Production tree is committed and pushed. The audit ledgers were subsequently committed to the development branch by user request; do not include them in release assets.
- Final verification script `scripts/rebuild-and-verify-audit.sh` exited 0 after F-017: target build, browser installer scenarios, manager JS syntax, whitespace, and Cppcheck on modified firmware sources passed. No hardware checks were run.
- Upstream FatFs static analysis was run over `source/ff15/*.c`; it exited 0. Cppcheck reported one nullable-string false positive (`nul` is initialized as the zero terminator) and four apparent uninitialized-object warnings where FatFs assigns the object filesystem pointer before each call. Inspection of those call sites confirmed initialization; no new defect was confirmed. The repository disk I/O dispatch is hardware-specific, so no host media/fault-injection harness was established.
- Audit completion is bounded by explicit deferred items below; do not describe physical or hardware behavior as tested. Remaining continuation should focus on upstream/FatFs and settings recovery feasibility, then record whether any additional production changes are warranted.

## Codespace and devkitPro setup

The initial container lacked devkitARM. An initial standalone installer attempt against the official package bootstrap URL received HTTP 403. That unused installer was removed because repository workflows did not call it and the devcontainer provisions the toolchain directly from the official image. To make devkitARM survive a Codespace rebuild, the following repository config was added:

- `.devcontainer/Dockerfile`: `FROM devkitpro/devkitarm:20260610`, with `DEVKITPRO=/opt/devkitpro` and `DEVKITARM=/opt/devkitpro/devkitARM`.
- `.devcontainer/devcontainer.json`: builds that Dockerfile and exports both environment variables.
- `scripts/rebuild-and-verify-audit.sh`: when devkitARM is absent inside a Codespace, checks `gh` authentication and requests a Codespace rebuild; after reconnect, running it again builds without cleanup and runs browser, JS, diff, and optional Cppcheck checks. It does not stage, commit, push, or clean.

The user rebuilt the Codespace. Verified in the current container:

- `arm-none-eabi-gcc (devkitARM) 16.1.0`
- `/opt/devkitpro/devkitARM/gba_rules` exists.
- Production `make` succeeds and prints `built ... omega-de-ds-style-multisystem-kernel.gba` and `ROM fixed!`.

The Linux/Codespaces target is `.gba`. The Windows `build.bat` workflow performs an additional copy/packaging step to `ezkernelnew.bin`; README documents this distinction. Grit/theme generation remains Windows/PowerShell-oriented and was not executed.

Packages installed ad hoc in this Codespace after the rebuild (not persisted in the devcontainer image): `git-lfs` 3.3.0, GitHub CLI `gh` 2.23.0, and Poppler utilities. A fresh Codespace may need them again. `git-lfs` is required by the configured pre-push hook; the first push dry-run failed until it was installed. After installation, `git push --dry-run origin feature/mirrored-artwork` and a no-op `git push origin feature/mirrored-artwork` both succeeded (`Everything up-to-date`). `gh auth status` succeeds as `voluptuary` via `GITHUB_TOKEN` in this container. Do not print or expose the token.

During the prior session, scoped command approvals were saved for pushing `feature/mirrored-artwork` and creating the v7.4.1 release. Approval state may not persist into a future session. Follow the active tool/platform approval policy and the user's current instructions; do not request broad shell/network access. Ad hoc apt installs in the prior Codespace are not part of the checked-in environment.

## Release planning history (completed; do not repeat)

The release was created and subsequently corrected to use the cartridge-required `ezkernelnew.bin` asset. See the authoritative Cold-start brief above for tag, commit, asset size, checksum, notes, and URL. The original `.gba` attachment was removed. No further release creation is needed for documentation-only audit continuation.

## Implemented findings

- `F-001` low: stale browser installer test expectations used style filenames at v7.3 although manifest/packages are v7.4. Updated four expectations; browser scenarios pass.
- `F-003` high: SD sector read timed out, then copied stale SRAM as success; write timeout omitted `SD_Disable()`. Fixed both error paths in `source/Ezcard_OP.c`. Hardware behavior remains untested.
- `F-004` medium: folder-selection stacks held 100 slots despite deeper `currentpath` capacity, and path construction truncated silently. Arrays now use `MAX_path_len`; folder entry rejects an unrepresentable path and only updates `currentpath` after successful `f_chdir`.
- `F-005` medium: cheat filename copy could leave a non-terminated 100-byte buffer before `strlen`/extension replacement. Bounded and terminated the copy, checked length, and null-checks input.
- `F-007` medium: PSRAM patch writes failed to switch page at 8 MiB boundaries. Writes are chunked by bank and select each page; odd-address PSRAM patch writes are rejected.
- `F-008` medium: malformed odd-length GameID index could access past complete records; no-match path leaked open file. Reads are checked, loop uses returned complete pairs, and file is closed on error/no match.
- `F-010` medium: `game_folder_total` could be used uninitialized after directory open failure or a control-flow route back to SD. Initialized at declaration and at each SD recount.
- `F-012` high: malformed SD cheat files could overrun section/key/value buffers, create unterminated strings, and underflow a section lookup. Parser bounds and termination checks were added; overlong code rows are discarded. Full devkitARM build and focused Cppcheck pass; hardware cheat-file loading remains untested.
- `F-013` high: save loading ignored FatFs read status and copied stale/partial EWRAM data into cartridge SRAM. Loads now stage and validate up to 128 KiB before any SRAM write, and errors propagate to normal/NOR/FRAM callers. Production build and focused Cppcheck pass; cartridge SD/SRAM behavior remains untested.
- `F-014` high: save writers ignored FatFs write/close failures, while startup cleared the pending FRAM recovery marker regardless. Writers now check each result and byte count; recovery remains pending after write failure, and FRAM save failure is reported. Production build and focused Cppcheck pass; card failure behavior remains untested.
- `F-015` high: RTS state load/write paths ignored short/error I/O and could copy stale buffer data into SRAM or treat a truncated state as complete. Exact RTS size and I/O checks now gate page writes and launch; production build and focused Cppcheck pass, and hardware behavior remains untested.
- `F-016` medium: RTS and FRAM paths used `strlen` after copying an entire fixed-width filename without guaranteeing a terminator. They now validate the source bound and minimum extension length before copying/replacing the suffix; build and focused Cppcheck pass.
- `F-017` medium: final review found that failed first-time `.sav` creation could leave a truncated file which a later launch could mistake for an existing save. `SavefileWrite` now unlinks the newly created file after a short/error write or failed close. The final audit script, including production build, browser scenarios, JS syntax, whitespace, and Cppcheck on modified firmware files, passes.

## Deferred findings / risks

- `F-002` low: Cppcheck flags GNU `void *` arithmetic at memory-mapped cartridge addresses. Actual devkitARM build succeeds; no behavior defect established.
- `F-006` low/probable: duplicate localization JSON catalogs drift; authoritative consumer/source is unknown. Do not sync without identifying ownership.
- `F-009` low: FatFs allows 255-character names, but compact list records show only 99 characters. Existing list area has a 64 KiB PSRAM limit; full LFN expansion would exceed it. Needs a memory-compatible UI design.
- `F-011` medium, confirmed persistence failure handling: `Launcher_SaveUnifiedSettings` opens `SETTINGS.TXT` with `FA_CREATE_ALWAYS`, ignores `f_printf` and `f_close` errors, then removes legacy files and clears migration pending if opening succeeded. A failed/full-card write can truncate settings and destroy migration sources. This session confirmed `f_error(&FIL)` and `f_close()` expose write/flush status, but checking them alone cannot prevent truncation at open. FatFs `f_rename()` registers the new entry and removes the old one in separate operations, so staged replacement needs startup recovery. The existing temp-copy helper deletes its destination before rename and is not a safe precedent. No host FAT formatting tools (`mkfs.fat`, `mformat`, `mcopy`) were available, and `source/ff15/diskio.c` dispatches to target-specific backends. The finding remains unresolved; no speculative persistent-data change was made.
- Hardware-only validation remains unavailable for memory-mapped SD/PSRAM, ROM patching, IRQ/sleep/reset, timing, and physical cartridge behavior.

## Review and validation already performed

- Thumbnail/artwork rendering: traced mirrored/custom/GBA/extension fallback lookup, full-read validation, marked BMP transparency, cache sizing/invalidation, scaling/clipping/compositing, list+art restoration, horizontal/vertical carousel, and selection/navigation transitions. No rendering defect confirmed by source review; no physical hardware test.
- Settings reads/migration, favorites/recent paths, kernel copy, FatFs disk boundary, directory depth/path, cheat filename/index parsing, assembly entrypoints and patch-builder callers inspected to varying depth. The saved scope is not exhaustive.
- All four in-tree assembly sources inspected. The RTS in-game menu saves/restores the full register set around its internal call. IRQ/sleep/reset hardware effects have source review only.
- Shipped `docs/DS Style User Guide.pdf` is 22 pages. All pages have now been extracted and reviewed against source, including installation, navigation, settings, Mode B, artwork, supported formats, safety, uninstall, troubleshooting, and reporting. No discrepancy was confirmed in the reviewed guidance.
- Final verification after `F-017`: `scripts/rebuild-and-verify-audit.sh` exits 0. It confirms devkitARM 16.1.0/GBA rules, production build, browser installer scenarios, all manager JS syntax, `git diff --check`, and Cppcheck on modified firmware files. The independent focused Cppcheck run also exits 0 with only its informational `toomanyconfigs` notice. No hardware tests were run.
- Broad non-FatFs command completed with exit 0:
  `cppcheck --enable=warning,performance,portability --suppress=missingIncludeSystem --inline-suppr --template=gcc --max-configs=1 source/*.c`
  It reported expected GNU memory-address `void *` arithmetic in `Ezcard_OP.c`, `GBApatch.c`, and `ezkernelnew.c`, plus `toomanyconfigs` because only one preprocessor configuration was analyzed. No other diagnostics. This broad pass is supplementary; it does not cover `source/ff15`.
- `pdfinfo 'docs/DS Style User Guide.pdf'`: 22 pages, unencrypted. `pdftotext` is available now.
- Do not run `make clean`; the audit script deliberately builds without cleanup.

## Current audit phase status

1. Repository baseline/inventory: complete; branch and current dirty tree must still be freshly reconciled.
2. Source/comment/documentation audit: broad subsystem review completed for rendering, filesystem paths, settings, navigation, build/test workflows, assembly, README/changelog, and the full PDF. FatFs static analysis and the flagged call paths found no new confirmed defect. Settings fault-injection feasibility remains open; see F-011 and next actions.
3. Findings ledger: findings F-001 through F-017 are described in the progress file; F-002, F-006, F-009, and F-011 remain deferred/triage items.
4. Focused fixes: implemented F-001/F-003/F-004/F-005/F-007/F-008/F-010/F-012/F-013/F-014/F-015/F-016/F-017; F-011 remains deferred.
5. Validation: target build, browser/JS checks and Cppcheck have passed as above; no hardware test.
6. Adversarial diff review: completed for the production commit, including F-017; staged whitespace and full audit script passed.
7. Final quality gate: production validation and release publication complete; hardware-only validation unavailable and F-011 remains deferred.

## Exact next actions

1. On resume, reconcile live Git state against the latest branch commit and these tracked ledgers.
2. F-011 remains open: assess whether a minimal host FatFs fault-injection harness is practical before changing settings persistence. FatFs static analysis and the flagged call paths have been reviewed with no new confirmed defect. F-002, F-006, and F-009 remain documented triage/limitations. If continued review finds a concrete issue, implement and verify it, then publish a follow-up release as appropriate.
3. Do not rerun publication or alter tags/releases absent new justified production changes. Keep both audit ledgers current and never claim physical hardware validation.

The audit is **not complete yet**. Do not report it complete until the remaining review is resolved or clearly bounded, release artifacts are verified, and the final ledgers and Git state are accurate.
