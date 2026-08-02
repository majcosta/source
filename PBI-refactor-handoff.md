# Handoff: PreBattle Interface / battle-group state

**For:** a design pass on JA2 1.13's pre-battle / battle-dispatch layer.
**Repo:** `E:\source\source`, branch off `master`. C++17, 32-bit MSVC, no test suite.
**Line numbers are post-fix** (see "What already changed"), so anchors will drift — trust the
function names.

## What you are being asked for

Do **not** start editing. Produce a plan and prompt the maintainer with it, including the choices
they have to make. The subsystem is load-bearing, has no tests, and is reachable from four different
application builds. A wrong refactor here silently eats battles, which is worse than the bug that
started this.

The specific brief: the pre-battle interface (PBI) and the battle-group plumbing behind it model
"the current battle" as a scatter of single-slot globals, but the strategic layer can produce
several concurrent battles in one tick. Design something sane. Say what it costs.

## The actual invariant that is broken

There is no `BATTLE` object. There is an implicit one, spread across ~18 globals in 5 files, with
no owner, no lifetime, and no identity. "Which battle?" is answered by whichever global was written
last.

The strategic clock advances groups in a loop. Several groups can arrive in different sectors in the
same tick, each triggering an independent battle. Every one of them writes the same globals.

### The implicit battle state

| Global | Defined | Holds |
|---|---|---|
| `gpBattleGroup` | `Strategic/PreBattle Interface.cpp:124` | group the PBI is about |
| `gubPBSectorX/Y/Z` | `Strategic/PreBattle Interface.cpp:230` | where |
| `gubEnemyEncounterCode` | `Strategic/PreBattle Interface.cpp:209` | why (ambush / invasion / bloodcat / creature / …) — drives which PBI buttons are legal |
| explicit encounter code | same file, `Get/SetExplicitEnemyEncounterCode` | second, subtly different copy of the above |
| `gubSectorIDOfCreatureAttack` | `Strategic/Creature Spreading.cpp:143` | where, again, but only for battles with no group |
| `gfPreBattleInterfaceActive` | `Strategic/PreBattle Interface.cpp:149` | is a PBI on screen |
| `gfPersistantPBI` / `gfUsePersistantPBI` | `:204` / `:237` | two flags for one concept, set at different times |
| `gfCantRetreatInPBI` | `:234` | consumed-on-read, one shot |
| `gfBlitBattleSectorLocator` | `:228` | map icon |
| `gfEnteringMapScreen`, `gfEnteringMapScreenToEnterPreBattleInterface` | `:82` | deferral because the PBI only exists in mapscreen |
| `gfAutomaticallyStartAutoResolve`, `gfEnterAutoResolveMode` | `:77`, `:81` | autoresolve handoff |
| `gfTransitionMapscreenToAutoResolve` | `Strategic/mapscreen.cpp:471` | same, third copy |
| `fDisableMapInterfaceDueToBattle` | `Strategic/PreBattle Interface.cpp` | input lock while a merc quote plays |
| `gpPendingSimultaneousGroup`, `gubNumGroupsArrivedSimultaneously` | `Strategic/Strategic Movement.cpp:81`, `:102` | a *partial, same-sector-only* attempt at the very problem this handoff is about |
| `gpTacticalTraversalGroup`, `gfTacticalTraversal` | `Strategic/PreBattle Interface.cpp:73` | tactical-to-tactical sector crossing, special-cased through the same paths |
| `gCurrentIncident` | Flugente's incident log | accumulates per battle, `FinishIncident()` closes it |

`gpPendingSimultaneousGroup` is worth reading closely: `PossibleToCoordinateSimultaneousGroupArrivals`
already implements "hold this battle, ask the player to merge arrivals" — but only for groups landing
in the *same* sector. The cross-sector case is the hole.

### The five ways a battle reaches `InitPreBattleInterface`

They do not share a path, and they arrive with the globals in different states.

1. **Merc quote path.** `PrepareForPreBattleInterface` (`Strategic/Strategic Movement.cpp`) picks a
   merc, queues `QUOTE_ENEMY_PRESENCE` with `DIALOGUE_SPECIAL_EVENT_BEGINPREBATTLEINTERFACE`. When
   the face finishes talking, `Tactical/Dialogue Control.cpp:649` fires `FACE_TRIGGER_PREBATTLE_INT`
   and calls in. Group is passed as `uiUserData1` — **a `GROUP*` cast through `UINT32`**.
2. **Message-box path.** No conscious merc present → `NotifyPlayerOfInvasionByEnemyForces` raises a
   box whose OK callback is `TriggerPrebattleInterface`, which posts
   `DIALOGUE_SPECIAL_EVENT_TRIGGERPREBATTLEINTERFACE` onto the dialogue queue, popped at
   `Tactical/Dialogue Control.cpp:1022`. Arbitrary player-controlled delay between arm and fire.
   Flugente left a comment at that line about the `UINT32`→`GROUP*` cast being garbage. He is right.
3. **Direct.** `PrepareForPreBattleInterface`'s militia branch and its no-conscious-merc branch call
   `InitPreBattleInterface` inline.
4. **Deferred-from-tactical.** If `guiCurrentScreen == GAME_SCREEN`, `InitPreBattleInterface` stores
   state, sets `gfEnteringMapScreenToEnterPreBattleInterface`, returns, and is re-entered from
   `HandlePreBattleInterfaceStates` (`Strategic/PreBattle Interface.cpp`, called once per frame from
   `Strategic/mapscreen.cpp:6066`).
5. **Non-persistent PBI.** `Strategic/mapscreen.cpp:6846` and `:14544` — the player clicking into a
   sector that already holds enemies. Different flag (`fPersistantPBI = FALSE`), same globals.
   Creature attacks (`Strategic/Creature Spreading.cpp:1210,1214,1413,1417`) and
   `Strategic/Ja25 Strategic Ai.cpp:2379` add more.

### The failure that started this

Enemy takes Drassen airport (militia only, no merc → forced autoresolve) in the same tick as a
2-merc group moving C9→C8 runs into enemies. Three independent breakages, all the same shape:

- `InitPreBattleInterface` opened with `if (gfPreBattleInterfaceActive) return;` — the second battle
  was **discarded**, no queue, no retry.
- `gpInitPrebattleGroup` was one `GROUP*` written by every pending battle, so the Drassen message
  box, acknowledged later, handed C8's group to the PBI.
- `gfDelayAutoResolveStart` was one BOOLEAN meaning "the next PBI goes straight to autoresolve". The
  merc PBI consumed it and autoresolved a battle the player should have been able to retreat from.
  **This is the symptom the maintainer originally reported.**

## What already changed (keep it; it is also the spec)

These are in the working tree, build clean, and are deliberately minimal. Treat them as an executable
description of the requirements, not as the design.

- `gfDelayAutoResolveStart` → `std::set<UINT8> gForcedAutoResolveSectors` (`Strategic Movement.cpp`).
  Keyed by sector, and a set because several can be armed at once.
- `gpInitPrebattleGroup` → `std::queue<UINT8> gPrebattleTriggerGroups`. Group **IDs**, not pointers,
  resolved via `GetGroup()` at fire time, so a group destroyed while its message box is up yields
  NULL instead of a dangling read. This is the fix Flugente's comment asks for.
- `NotifyPlayerOfInvasionByEnemyForces` now returns whether a box with a callback actually went up.
  The wilderness case only prints a `ScreenMsg` — no box, no callback — which is why the caller also
  triggers directly. For SAM sites and militia groups inside towns, **both** fired: two PBI triggers
  for one battle. Now exactly one.
- `InitPreBattleInterface` queues instead of dropping. Two queues: `gPendingAutoResolveBattles`
  drains before `gPendingBattles`, per the maintainer's rule — battles the player cannot take to
  tactical resolve first, because a tactical fight can run for a long time and everything behind it
  is memory-only. A tactical-capable battle also defers while any battle is armed and waiting on a
  message box.
- Each queue entry snapshots group ID, encounter code, `gubSectorIDOfCreatureAttack`, persistent
  flag — precisely because the battle that goes first overwrites all of them. **That snapshot list is
  the minimum viable `BATTLE` object.** Read it as the requirement.
- `RemoveAllGroups` clears all three containers (new game / load).

### Known hole, and the maintainer's assumption to check

Two **merc-involved** battles at once: neither gets priority, the second sits in `gPendingBattles`,
the player enters tactical for the first, saves mid-fight, reloads — the queue is gone. The
maintainer's read was "no fix except changing savegames".

**That assumption looks wrong, and is worth leading with.** `GENERAL_SAVE_INFO`
(`Ja2/SaveLoadGame.cpp`, ends ~`:459`) carries `UINT8 ubFiller[265]` with the comment
*"This structure should be 1588 bytes"* — reserved space, already written and read. A small pending-
battle queue fits there with no size change and no format break; old saves read zeros and yield an
empty queue.

Separately, this project *does* version its save format deliberately: `SAVE_GAME_VERSION` in
`Ja2/GameVersion.h:110` is a named constant in a documented list of revisions, and loaders branch on
`guiCurrentSaveGameVersion >= SOME_NAMED_CHANGE` throughout `SaveLoadGame.cpp`. So a format bump is a
supported, routine operation here, not a catastrophe. Both routes are open. Present the trade.

A third route worth pricing: don't persist at all, **re-derive on load**. An enemy group and a player
group co-located in a sector with no battle running is already in the saved group list; a post-load
sweep could re-arm through the existing `CheckConditionsForBattle`. Cost: the encounter code is
reconstructed rather than restored, so an ambush reloads as a plain encounter.

## Constraints you must respect

- **No test suite.** CI only compiles. Verification is a human loading a save. Any plan must say how
  each step gets verified, and must be shippable in stages that individually make sense.
- **Four application builds** from one tree (`JA2`, `JA2MAPEDITOR`, `JA2UB`, `JA2UBMAPEDITOR`),
  switched by preprocessor, not runtime flags. Code inside `#ifdef JA2UB` / `#ifdef JA2EDITOR` only
  compiles in that variant — a refactor that builds for `JA2` can break the other three. CI builds
  all four; build them locally too.
- **Savegame compatibility** is a real user-facing constraint (people have long campaigns running).
- **`GROUP*` lifetime.** Groups are `MemFree`'d by `RemovePGroup`. Anything that outlives a frame
  must hold `ubGroupID`, not a pointer. Several existing globals violate this; the dialogue queue
  still passes a `GROUP*` through a `UINT32`.
- Many files are **windows-1252**, not UTF-8. Filenames contain spaces. Mixed CRLF/LF — match the
  file you are editing.
- The maintainer works in this code daily and has a save that reproduces the original bug, and will
  set breakpoints on request. Ask for that rather than guessing.

## Decisions to put to the maintainer

1. **Scope.** Introduce a real `BATTLE` record and a dispatcher owning the queue, or keep the flat
   globals and only fix identity/lifetime? The former is the right shape; the latter is what ships
   without a month of regression hunting. There is a middle: a `BATTLE` struct that the existing
   globals are *derived from* on activation, so downstream code is untouched.
2. **Persistence route.** `ubFiller` (no format change) vs. a versioned format bump vs. re-derive on
   load. Recommend one.
3. **Ordering policy.** Currently: forced-autoresolve first, then FIFO. Is that final, or should the
   player get to pick the order when several battles land at once (the
   `PossibleToCoordinateSimultaneousGroupArrivals` prompt already sets a precedent)?
4. **How far into the entry points to go.** Collapsing the five paths into one is most of the value
   and most of the risk. Non-persistent PBI (path 5) and tactical traversal may deserve to stay
   separate.
5. **`gfPersistantPBI` vs `gfUsePersistantPBI`, and the two encounter codes.** Are these genuinely
   two concepts each, or historical accidents? The maintainer will know; do not guess.

## Build

Needs an **x86** MSVC environment. On this machine:

```bash
cmd /c '"E:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars32.bat" >nul && cd /d "E:\source\source\build\1dot13 RelWithDebInfo" && cmake --build . --target JA2'
```

Output goes to `%JA2_GAMEDIR%` (currently `e:\bugfix\JA2.exe`), so the maintainer can run a save
against it directly. Configure from scratch per `CLAUDE.md` if the build tree is missing.
