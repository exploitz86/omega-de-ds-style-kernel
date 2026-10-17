Startup/thumbnail timing diagnostic candidates

Corrected candidates

U6  ezkernelnew-timing-U6-post-loader-trace.bin
    Preserves T5's custom-loader stub and 176-byte stack probe, then adds
    checkpoints after the thumbnail path, drawing, and adjacent-preview warmup.
    Size: 2,089,848 bytes. SHA-256:
    a548db64dd2ae8004ff3c48b90e99bae138cc31eba2af85cb9d66ca12f09f397

U7  ezkernelnew-timing-U7-post-header-trace.bin
    Preserves U6's custom-loader stub and 176-byte stack probe. Adds two-digit
    checkpoints around the GBA ROM-header read, game-code art lookup, extension
    art, fallback image, and return from the path loader.
    Size: 2,089,968 bytes. SHA-256:
    3f19e392b5dca2563733a36a6833fa3d6264576a96b00a9b111673c9a001ba4f

U8  ezkernelnew-timing-U8-gamecode-lookup-trace.bin
    Preserves U7's stub/probe and adds internal markers around game-code art
    lookup entry, path/open, audio stop, thumbnail read/tagging, close, and return.
    Size: 2,090,224 bytes. SHA-256:
    36f98073b2c2cda0976b2625cc5d95ff2b6ae261d9019c50050ab78de0716cf6

U9  ezkernelnew-timing-U9-path-format-trace.bin
    Preserves U8's lookup trace and adds a marker immediately before path
    formatting, separating it from the following f_open call.
    Size: 2,090,232 bytes. SHA-256:
    3a44f757fba3e81d7b0f074a2aefdd36b9b9fe1784ee66b81b5af91ef84c5c71

R2  ezkernelnew-timing-R2-after-recovery.bin
    Adds 60 VBlank frames (about one second) after save recovery.
    SHA-256: 88dcc7227044f7c7a316e3991a217e5d99c5e4381653c2674c04a25d35024a0e

T2  ezkernelnew-timing-T2-before-preview.bin
    Adds 60 VBlank frames (about one second) immediately before the start-screen
    recent-title lookup and thumbnail loader.
    SHA-256: 3266809aa9033f0d067575ca84ee390a3613138b75eb7ea2d12c5c3d42fd5442

T3  ezkernelnew-timing-T3-loader-entry-trace.bin
    Keeps T2's 60-frame wait and adds checkpoints through the thumbnail loader.
    SHA-256: a34ffcc699271432d7d7f3b1906d53c704c4c27e3e9f542bfd62d80a3ae50763

T4  ezkernelnew-timing-T4-custom-loader-bypass.bin
    Keeps T3's wait and trace, but replaces the custom-thumbnail loader with a
    tiny stub that marks entry and returns "not found".
    SHA-256: cd38f0d6c17abb0ca771ba1ed6090246da5430ecdb147120aba6c9f2c3668e0a

T5  ezkernelnew-timing-T5-stack-frame.bin
    Keeps T4's custom-loader bypass but adds a volatile 176-byte stack probe to
    approximate the former loader's roughly 200-byte static frame. Its visible
    marker prefix is U5, to distinguish this image from earlier T builds.
    SHA-256: a6c2a3946e88f94f4c5412bece446fab20afb44f4befe85b94ebfd7755abeb55

These images clear a black marker box before drawing and use numeric phase codes.
The marker appears near the lower-left of the splash and in the start-screen top
bar. Its first two characters identify the image; the last digit shows the last
phase reached.

R2/T2 phase digits (the first two characters identify the build)

0  splash/startup beginning
1  entering settings migration/save
2  returned from settings migration/save
3  entering save recovery
4  returned from save recovery; R2's wait follows this point
5  entering the boot-time last-played lookup
6  boot lookup returned or was skipped; also used when there is no preview path
7  entering the start-screen preview lookup; T2's wait runs at this phase
8  preview load returned
9  caller-side marker immediately before Load_ThumbnailEx or cached-favourite
   preview loading; it does not prove the loader's first instruction ran

T3 phase digits

0  splash/startup beginning
1  start-screen recent-title path reached; the 60-frame wait runs here and this
   marker remains visible until the loader call
2  first statement in Load_ThumbnailEx
3  path/current-directory resolution finished, before the path-loader call
4  entered Launcher_LoadThumbnailForPath
5  mirrored-art lookup begins
6  mirrored-art lookup missed
7  immediately before custom-title thumbnail lookup
8  custom-title loader entry, before thumbnail-size and manifest checks; also
   reused after some paths return
9  thumbnail-size/manifest checks passed, immediately before custom path setup
   and f_open

These are visible checkpoints, not proof of the underlying hardware operation
that stalled. For example, T32 proves Load_ThumbnailEx began; T31 means execution
did not visibly reach its first statement. T38 versus T39 separates the custom
loader checks from path setup/open.

T4 uses the T3 phases 0-7. Its phase 8 is entry to the tiny custom-loader stub;
phase 9 means the stub returned to Launcher_LoadThumbnailForPath. The stub skips
custom-thumbnail manifest lookup and file reading, so this diagnostic is not
expected to display a custom thumbnail. T4 still performs the mirrored-art
lookup.

T5 uses the same visible phases as T4, with the distinct U5 prefix. Phase 8
marks entry to a stub that declares and touches a volatile 176-byte stack probe
before returning; phase 9 means the caller observed the return. This approximates
the former custom loader's static frame but does not reproduce its body or prove
the cause. The custom console-folder art is still intentionally bypassed, while
mirrored game artwork remains enabled.

First-set results

C0 and S1 booted normally. R1 also booted normally but showed a different phase
glyph. T1 stalled during boot. The first-set ID prefixes were readable, but its
phase glyphs were drawn transparently without erasing older pixels, so do not use
their final glyph to identify a phase. The R1/T1 outcomes remain useful because
their build IDs confirmed which images were running.

Hardware results (2026-10-15)

R2 booted normally, displayed R28, and worked through different menus and game
loads. T2 failed to boot normally: it displayed T20, then T26 and T27 briefly,
then T29 and froze. T2 phase 9 is only the caller-side marker before the
thumbnail-loader call; it does not establish that the loader was entered. The
different outcomes show that the one-second delay after recovery is harmless in
the tested setup, while delaying immediately before the preview path reproduces
the stall. They do not identify the cause or prove a SETTINGS.TXT race.

T3 failed to boot normally: it displayed T30, then T31 briefly, then T37 and
froze. This shows the delay completed, the outer thumbnail loader ran, path
resolution completed, mirrored-art lookup missed, and the code reached the
caller marker just before calling Launcher_LoadCustomThumbnailByName. T38, the
first marker inside that function, was not visible. This narrows the observed
boundary to the call/entry interval, but does not prove stack overflow or name
the exact failing operation.

T4 hardware result: it booted to the start screen and mirrored game artwork
loaded. Custom console-folder artwork from IMGS2\CUSTOM was absent and the
16x14 system folder icon appeared, as expected because T4 bypasses that loader.
The user reported markers T40, T31 briefly, then T39. T40 and T39 indicate the
stub was entered and returned; T31 does not match the T4 prefix and should be
treated as an ambiguous visual reading. The successful boot and mirrored game
art weigh against mirrored lookup alone causing the stall.

T5 hardware result: the user reports U50, U51 briefly, then U59 followed by a
freeze instead of normal startup. U59 means the caller observed the stub return;
the failure therefore continued beyond that visible checkpoint in this run.
This does not establish that the probe caused the freeze, prove stack overflow,
or identify the next failing operation. The diagnostic remains non-production.

U6 uses the same volatile stack probe and stub as T5, but adds later checkpoints.
All marks are drawn in the top-left marker box. Its phase digits are stage IDs,
so the order jumps from 9 back to 2 after the stub-return point:

0  entered Launcher_DrawStartLastThumb
1  recent-title lookup returned an entry
8  entered Launcher_LoadCustomThumbnailByName, before touching the probe
9  caller observed the custom-loader stub return
2  Launcher_LoadThumbnailForPath returned to Load_ThumbnailEx
3  Load_ThumbnailEx returned to Launcher_DrawStartLastThumb
4  thumbnail drawing/scaling and corner restoration completed
5  start-screen options drawn; adjacent-preview warmup is next
6  adjacent-preview warmup returned
7  Launcher_DrawStartWindow returned to Launcher_StartWindow

U6 hardware result: the user reports it stalls at U69. U69 means the custom
thumbnail stub returned false; U62 would mark `Load_ThumbnailEx` returning after
the rest of `Launcher_LoadThumbnailForPath`. The remaining path includes the GBA
header probe/game-code artwork attempt, extension-default artwork, and the
not-found fallback where applicable. The marker does not identify which step
stalled and does not establish stack overflow or a production regression. U6
still bypasses custom console-folder artwork; its absence is expected. U62/U63/
U64/U65/U66/U67 are later checkpoints that were not reported in this run.

U7 preserves the stub/probe and uses two-digit stage IDs after the return point:

00  entered Launcher_DrawStartLastThumb
01  a recent-title entry was found
02  Load_ThumbnailEx returned to Launcher_DrawStartLastThumb
03  thumbnail drawing/scaling and corner restoration completed
04  start-screen options drawn; adjacent-preview warmup is next
05  adjacent-preview warmup returned
06  Launcher_DrawStartWindow returned to Launcher_StartWindow
08  entered the custom-thumbnail stub
09  the stub returned false to Launcher_LoadThumbnailForPath
10  filename was identified as GBA
11  ROM path was built, immediately before opening the ROM
12  f_open returned (success or error)
13  ROM header seek/read and close completed
14  a valid game code was read; game-code artwork lookup is next
15  game-code artwork lookup returned
16  ROM-header helper returned to Launcher_LoadThumbnailForPath
17  extension-default artwork lookup returned
18  fallback thumbnail construction completed, immediately before path-loader return
19  Launcher_LoadThumbnailForPath returned to Load_ThumbnailEx

On U7, a stop at U709 still means the stub return marker was reached. U711
through U713 narrow the ROM open/header-read interval; U714 without U715 isolates
the game-code artwork lookup; U717 marks the extension-art attempt; U718 means
the not-found fallback completed; U719 means the path loader returned. U7 is
diagnostic-only and its extra marker drawing can perturb execution. The probe
also remains an approximation of the old loader's stack frame, not its body.

U7 hardware result: the user reports a stall at U714. That marker follows a
successful four-byte game-code read and appears immediately before
Launcher_LoadThumbnailByGameCode; U715, after the lookup returns, was not
observed. This narrows the visible interval to the call/setup/lookup/return path
but does not prove the helper's first instruction ran or identify the failing
operation. Inspect the lookup and use U8 internal markers to narrow it further.

U8 uses these phases inside the game-code lookup (the visible prefix is U8).
The phase 14/15 caller markers remain around the lookup:

20  first statement in Launcher_LoadThumbnailByGameCode, before size calculation
21  thumbnail read size calculated
22  game-code thumbnail path formatted, immediately before f_open
23  f_open returned (success or error)
32  invalid arguments or f_open error branch, immediately before returning miss
24  shared-buffer audio stop returned
25  Launcher_ReadThumbnailBytes entered
26  f_read (or the chunk-read loop) returned
27  transparent-pixel tagging began after a complete successful read
28  transparent-pixel tagging returned
29  Launcher_ReadThumbnailBytes returned to the game-code lookup
30  f_close returned
31  lookup result computed, immediately before helper return
15  caller observed the lookup return

Read/tag markers are enabled only during this lookup, so the earlier GBA-header
read does not emit phases 25–28. U820 without U821 points to the size-helper
interval; U822 without U823 to f_open; U823 without U824 to audio stop; U825
without U826 to the read; U827 without U828 to transparency tagging; U829
without U830 to close. U831 or U815 means the lookup returned. U832 is the
invalid/open-error branch; a normal miss should then reach U815. U8 still draws
markers and retains the stack probe, so these observations may perturb timing or
stack use. To run a candidate, use the usual cartridge update procedure with the
candidate renamed to `ezkernelnew.bin`, keep the known-good image available, and
report the last visible marker.

U8 hardware result: the user reports it stalls at U821. U821 is drawn after the
thumbnail-size calculation; U822, immediately after path formatting, was not
reported. The visible interval includes possible completion of drawing U821,
the `snprintf` call, and drawing U822, so this does not prove that formatting
itself stalled. U9 added explicit markers around `snprintf` and `f_open`.

U9 retains U8's phase meaning where applicable, with these path/open boundaries:

20  first statement in game-code lookup, before thumbnail-size calculation
21  thumbnail size calculated
22  immediately before `snprintf` path formatting
23  `snprintf` returned; immediately before `f_open`
24  `f_open` returned (success or error)
33  invalid arguments or open-error branch, immediately before returning miss
25  shared-buffer audio stop returned
26  thumbnail read helper entered
27  read or chunk-read loop returned
28  transparent-pixel tagging began
29  transparent-pixel tagging returned
30  read helper returned
31  `f_close` returned
32  lookup result computed, immediately before helper return
15  caller observed the lookup return

U922 without U923 places the visible interval around completion of the U922
marker draw, the formatter, or drawing U923. The U922 marker is emitted by a
call that clears a small screen region and draws text; seeing the marker does
not prove that call returned. U923 without U924 places the interval around
`f_open` or drawing U924. U9 retains the marker renderer and stack probe;
markers are observational and may perturb execution.

U9 hardware result: the user reports a stall at U922. The marker call starts
just before `snprintf`; U923 was not reported, and `f_open` follows U923. The
stall may be in completion of the U922 draw, the formatter, or the U923 draw;
it does not prove the formatter itself stalled. The user is concerned about flash-chip
wear from repeated candidate flashing. Do not build additional trace images for
this boundary; continue with offline source/disassembly/host checks and only
consider a consolidated candidate after a concrete change is justified and
locally validated.

The original C0/S1/R1/T1 binaries remain in this folder for reference. R2, T2,
T3, T4, T5, U6, U7, U8, and U9 are retained for comparison. The user has tested
T5, U6, U7, U8, and U9; no more U9 trace builds are planned. All timing traces
are diagnostic-only and have not been qualified for release.
