# Plan: PBI battle queue — middle scope, written for execution by Opus 5

**Executor:** Opus 5. Follow steps in order. Every step ends with a build of all four apps and a
named human check. Where a step says STOP-AND-ASK, stop and ask the maintainer — do not guess.

**Companion doc:** `PBI-refactor-handoff.md` — problem statement, the 18-global inventory, the five
entry paths, constraints. Read it first. **One correction to it:** the handoff says the working-tree
diff is "the spec — keep it". That is now wrong, see Status.

## Status and decisions (revised 2026-08-01)

- **Scope: middle.** A `BATTLE` snapshot record + one queue + one activation point that *derives*
  today's globals. Downstream readers untouched. The full dispatcher (globals deleted, state
  machine) is parked — appendix.
- **The working-tree diff is BROKEN. Do not keep it.** Observed on the repro save: "Enemy forces
  have invaded B13: Drassen Airport" message appears, the forced-autoresolve PBI for Drassen never
  shows, the C8-C9 merc PBI comes up instead. The diff is a previous model's attempt; treat it as
  reference for *intent* only. Two defects already identified by inspection:
  - The `guiCurrentScreen == GAME_SCREEN` deferral inside `InitPreBattleInterface` is still
    single-slot (`gpBattleGroup` + `gfUsePersistantPBI`): two battles deferring in one tick, second
    overwrites the first — battle eaten.
  - Queue entries snapshot `GetEnemyEncounterCode()` (base) but restore via
    `SetExplicitEnemyEncounterCode()` (the superset code). Mismatched ranges.
- **Persistence:** into `GENERAL_SAVE_INFO.ubFiller[265]`, no format bump (maintainer decision).
- **Ordering:** forced-autoresolve battles first, then FIFO (maintainer decision, final).
- **Flag verdict (investigated):** `gfPersistantPBI` genuine (active-battle property, save-persisted,
  ~35 readers incl. 12 in `Tactical/Soldier Create.cpp`) — KEEP as global in this scope.
  `gfUsePersistantPBI` is an accident — it is the `fPersistantPBI` argument smuggled through the
  deferred-to-mapscreen replay (`HandlePreBattleInterfaceStates` reads it only to pick which
  `InitPreBattleInterface` call to replay). It dies naturally in step 2b.
  Encounter codes are genuinely two ranges: explicit is a documented superset
  (`PreBattle Interface.h:39`) and is already saved. Snapshot BOTH, restore BOTH.

## Decisions added after the step-1 trace (2026-08-01)

- **Execution order changed: 3a, 3b, then step 2, then 3c, 4, 5.** Guard 5 of `TryActivateNextBattle`
  needs a truthful "a trigger is armed and unacknowledged" signal. `gpInitPrebattleGroup` is not one
  (see Trace results). `gPrebattleTriggerGroups` is, so it must exist first.
- **The merc quote belongs to the battle, not to the tick** (maintainer): the merc is saying "a
  battle is about to take place *where I am*", so it must play immediately before *its own* PBI.
  This moves the quote from arming time to activation time — see 2a.
- **Repro save: player is in MAP_SCREEN** when the tick fires. The deferral path
  (`InitPreBattleInterface:373` / `:481`) is therefore not implicated in the original bug; it still
  has to be fixed (2b), but it is not what ate the Drassen battle.
- **Acceptance ordering (answers step 1's STOP-AND-ASK):** B13 box → OK → Drassen forced autoresolve
  → resolve → C8 activates → merc quote → C8 PBI with retreat legal. The quote follows its battle.

## Ground rules — hard constraints for the executor

1. No code edits before step 0 and step 1 are complete.
2. After every step, build **all four** applications. From this machine:
   ```
   cmd /c '"E:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars32.bat" >nul && cd /d "E:\source\source\build\1dot13 RelWithDebInfo" && cmake --build .'
   ```
   (no `--target` = all configured apps; if the tree only configures JA2, configure the rest per
   `CLAUDE.md`). Output lands in `%JA2_GAMEDIR%` for the maintainer to run.
3. Never store a `GROUP*` across a frame or through a queue/callback. `ubGroupID` only, resolve
   with `GetGroup()` at use time, tolerate NULL (group destroyed = battle skipped, not crash).
4. Do NOT touch: `PossibleToCoordinateSimultaneousGroupArrivals` (same-sector merge prompt),
   tactical traversal (`gfTacticalTraversal`, `gpTacticalTraversalGroup`), the player-clicked
   non-persistent PBI path (`mapscreen.cpp`), `Tactical/Soldier Create.cpp`,
   `Strategic/Auto Resolve.cpp` internals, anything under `Multiplayer/`.
5. Encoding: many files windows-1252, not UTF-8. Match each file's existing CRLF/LF. Filenames
   contain spaces — quote paths.
6. Line numbers below are for the CURRENT (diff-applied) tree and will shift after step 0.
   Anchor by function name + quoted landmark code, never by number alone.
7. There is no test suite. The named check at the end of each step IS the test. A step without its
   check passing does not proceed. The maintainer has a repro save and will run it or set
   breakpoints on request — ask, don't guess.

## Step 0 — park the broken diff, start clean

The repo is on `master` (verified: all plan-relevant files identical to the commit this plan was
written against; only cosmetic cast/comment changes elsewhere) with the broken attempt as
uncommitted modifications. Preserve it on a branch, then work from a clean `master`:

```
git switch -c opus-attempt
git add "Strategic/Map Screen Interface.cpp" "Strategic/Map Screen Interface.h" "Strategic/PreBattle Interface.cpp" "Strategic/PreBattle Interface.h" "Strategic/Strategic Movement.cpp"
git commit -m "opus attempt at PBI battle queueing (broken: forced autoresolve skipped)"
git switch -c pbi-queue master
```

The two `.md` files stay untracked and travel with the working tree.
*Check:* `git status` on `pbi-queue` shows only the untracked `.md` files; `git diff master` empty.

## Step 1 — trace the repro tick on clean master (no fixes yet)

Goal: an exact event-order record of the failing tick (Drassen B13 militia battle + C8-C9 merc
battle), because the arming order dictates step 2's guard conditions.

Ask the maintainer to run the repro save with breakpoints (or add temporary `DebugMsg`/`ScreenMsg`
logging, removed afterwards) at:

- `CheckConditionsForBattle` (`Strategic/Strategic Movement.cpp`) — log group ID, sector,
  `fCombatAbleMerc`, `fMilitiaPresent`, which branch taken.
- `PrepareForPreBattleInterface` — log which of its three branches fires (militia / quote / direct).
- `InitPreBattleInterface` entry — log args, `guiCurrentScreen`, `gfPreBattleInterfaceActive`.
- `TriggerPrebattleInterface` — log `gpInitPrebattleGroup` value at call.
- Both dialogue-queue landings in `Tactical/Dialogue Control.cpp`: the
  `FACE_TRIGGER_PREBATTLE_INT` block (landmark: `InitPreBattleInterface( (GROUP*)gpCurrentTalkingFace->uiUserData1`)
  and the `DIALOGUE_SPECIAL_EVENT_TRIGGERPREBATTLEINTERFACE` block (landmark: Flugente's
  "hideous idiocy" comment).
- `HandlePreBattleInterfaceStates` — the `gfEnteringMapScreenToEnterPreBattleInterface` replay branch.

Record and append to this file under a "## Trace results" heading:

1. Which battle arms first in the tick, and from which branch.
2. Is the player in GAME_SCREEN or MAP_SCREEN when the tick fires.
3. Does B13 raise a message box (callback armed) or only a `ScreenMsg`.
4. On master, what exactly kills the Drassen battle (expected per handoff: the single
   `gfDelayAutoResolveStart` is consumed by the C8 PBI, and/or `gpInitPrebattleGroup`
   overwritten — confirm which).

Also confirm the acceptance ordering with the maintainer (STOP-AND-ASK, one question):
expected UX is *B13 box → OK → Drassen autoresolve PBI → resolve → C8-C9 PBI with retreat legal* —
i.e. does the merc quote for C8 play before or after the Drassen autoresolve? Write the answer down.

*Check:* trace recorded in this file; maintainer confirms it matches what they see on screen.

## Trace results

**Status: static trace, derived by reading clean `master` (branch `pbi-queue`, identical to
`d683cdeb6`). Not yet confirmed on the repro save.** Line numbers are clean-master.

### The tick, step by step

Both battles are raised from the group-arrival loop, `Strategic Movement.cpp:2238`
(`if( fCheckForBattle && !CheckConditionsForBattle( pGroup ) && !gfWaitingForInput )`). The loop
keeps advancing groups after a battle arms, which is what makes the tick contain two.

**B13 Drassen Airport (militia only, enemy group arrives) — `CheckConditionsForBattle`:**

| line | what happens |
|---|---|
| `1302` | `pGroup->usGroupTeam == ENEMY_TEAM` branch, militia in sector → `fMilitiaPresent = fBattlePending = TRUE` |
| — | no player group in B13 → `fAliveMerc = fCombatAbleMerc = FALSE`, `pPlayerDialogGroup == NULL` |
| `1369` | `gpInitPrebattleGroup = pGroup` (B13 enemy group) |
| `1380-1381` | `gfDelayAutoResolveStart = TRUE; gfUsePersistantPBI = TRUE` |
| `1384` | `NotifyPlayerOfInvasionByEnemyForces(..., TriggerPrebattleInterface)` → `Map Screen Interface.cpp:6346` `bTownId` branch → **message box with callback**. This is the observed "Enemy forces have invaded B13: Drassen Airport" (`pMapErrorString[23]`) |
| `1387` | `usGroupTeam == ENEMY_TEAM` and B13 is a town sector → the direct `TriggerPrebattleInterface(1)` is **skipped**. Box only. |
| `1411` | `pPlayerDialogGroup == NULL` → no `PrepareForPreBattleInterface`. Returns TRUE. |

So B13 never reaches `InitPreBattleInterface` in this tick. It is *armed only*: box on screen,
`gpInitPrebattleGroup` = its group, `gfDelayAutoResolveStart` set.

**C9→C8 merc group, same tick, same function:**

| line | what happens |
|---|---|
| `1267` | `usGroupTeam == OUR_TEAM`, enemies in C8 → `fBattlePending = TRUE`, `pPlayerDialogGroup = pGroup`, `fCombatAbleMerc = TRUE` |
| `1369` | `gpInitPrebattleGroup = pGroup` — **overwrites B13's group** |
| `1377` | `fCombatAbleMerc` is TRUE → autoresolve branch skipped; `gfDelayAutoResolveStart` **stays TRUE from B13** |
| `1414` | `PrepareForPreBattleInterface` → merc-quote path (`1142`), `fDisableMapInterfaceDueToBattle = TRUE`, quote queued |
| — | quote ends → `Dialogue Control.cpp:646` `FACE_TRIGGER_PREBATTLE_INT` → `InitPreBattleInterface( C8 group, TRUE )` → C8 PBI on screen |
| `PreBattle Interface.cpp:2698` | `gfDelayAutoResolveStart && gfPreBattleInterfaceActive` → **C8 autoresolves**, retreat never offered |

### Answers to the four questions

1. **Which arms first:** B13, from the `CheckConditionsForBattle` `!fCombatAbleMerc` +
   `fMilitiaPresent` branch (message-box path). C8 arms second, merc-quote path.
   *Caveat:* order is group-list iteration order, not guaranteed. The maintainer's observation
   (box text appears, then the merc PBI) says B13 first on the repro save.
2. **Screen:** MAP_SCREEN (maintainer). So neither deferral branch (`InitPreBattleInterface:373`,
   `:481`) runs in the repro tick — the single-slot deferral is a real bug but not this one.
3. **Does B13 raise a box:** yes, with a callback — `bTownId` branch, `Map Screen Interface.cpp:6346`.
   Not the wilderness `ScreenMsg` case.
4. **What kills the Drassen battle:** *both* named mechanisms, and they are separable:
   - `gfDelayAutoResolveStart` is set by B13 and consumed by the C8 PBI
     (`PreBattle Interface.cpp:2698`) → the reported symptom (no retreat on the merc battle).
   - `gpInitPrebattleGroup` is overwritten at `1369` by C8. When the player OKs the B13 box,
     `TriggerPrebattleInterface` (`1421`) posts **C8's** group, then NULLs the slot → B13's battle
     is gone entirely.

### Plan defect found while tracing — step ordering

Step 2a guard 5 says: before step 3 lands, read the master equivalent `gpInitPrebattleGroup != NULL`
as "an armed trigger is outstanding". **That proxy is false on master.** `gpInitPrebattleGroup` is
written at `1369` for *every* pending battle, including merc-quote battles that never call
`TriggerPrebattleInterface` — nothing else in the tree clears it (grep: only `1425`; `6199` is
commented out). So in the repro tick, C8 sets it for itself, and guard 5 would defer C8 on its own
armed flag until the *unrelated* B13 box is acknowledged.

**Recommendation: land 3a and 3b before step 2.** `gPrebattleTriggerGroups` is only pushed at the
sites that actually arm a trigger, so it is a truthful signal for guard 5; `gpInitPrebattleGroup`
never was. 3a/3b are independently verifiable on master (their checks do not need the queue) and
were the sound part of the parked branch. Cost of not reordering: guard 5 ships wrong for one step
and step 2b's two-battle check is unreliable.

## Step 2 — BATTLE snapshot, one queue, one activation point

All in `Strategic/PreBattle Interface.cpp` unless noted.

### 2a. The record and the queue (behavior-preserving skeleton)

```cpp
struct BATTLE
{
	UINT8	ubGroupID;					// 0 = groupless (creature attack, airdrop, militia-only transfer)
	UINT8	ubSectorID;					// SECTOR(x,y) of the battle; replaces per-path gubSectorIDOfCreatureAttack reliance
	INT8	bSectorZ;
	UINT8	ubExplicitEncounterCode;	// snapshot of GetExplicitEnemyEncounterCode()
	UINT8	ubEncounterCode;			// snapshot of GetEnemyEncounterCode() — BOTH, see Status
	BOOLEAN	fPersistant;
	BOOLEAN	fForcedAutoResolve;			// battle must go straight to autoresolve (no combat-able merc)
	BOOLEAN	fCantRetreat;				// snapshot of gfCantRetreatInPBI
	UINT8	ubTalkerID;					// merc who pipes up when this battle activates; NOBODY = silent
};
static std::deque<BATTLE> gPendingBattles;	// single container; pick rule supplies the ordering
```

- `InitPreBattleInterface(pBattleGroup, fPersistantPBI)` becomes: existing validity checks →
  build a `BATTLE` snapshot (group ID or 0; sector from group or `gubSectorIDOfCreatureAttack`;
  both encounter codes; `fForcedAutoResolve` from the arming site, see 2d) → push →
  `TryActivateNextBattle()`.
- **Quote at activation.** `PrepareForPreBattleInterface`'s merc branch (`Strategic Movement.cpp:1142`)
  today picks a merc, calls `HandleImportantPBIQuote`, sets `fDisableMapInterfaceDueToBattle` and
  pauses — all at arming time, with the quote acting as the trigger that later reaches
  `InitPreBattleInterface` via `FACE_TRIGGER_PREBATTLE_INT`. Invert it: the branch snapshots the
  chosen merc into `BATTLE::ubTalkerID` and enqueues, nothing else. `ActivateBattle` splits in two:
  if the record names a talker who is still `bLife >= OKLIFE`, set `fDisableMapInterfaceDueToBattle`,
  pause, `HandleImportantPBIQuote`, return — the PBI half runs from the `FACE_TRIGGER_PREBATTLE_INT`
  landing in `Dialogue Control.cpp:646` against `gActiveBattle`, not by re-entering the enqueue path.
  Guard 3 then supplies the co-timing for free: nothing else activates while a merc is talking.
  The `gfTacticalTraversal` branch above it (`1108-1138`) is untouched — it plays a quote and
  deliberately never raises a PBI. This subsumes the face half of 3c and removes entry path 1.
- `ActivateBattle(const BATTLE&)` = the current body of `InitPreBattleInterface` from
  `gfPersistantPBI = fPersistantPBI;` onward, prefixed by writing the globals from the record:
  `gpBattleGroup = GetGroup(id)/NULL`, `gubPBSectorX/Y/Z`, `SetEnemyEncounterCode`,
  `SetExplicitEnemyEncounterCode`, `gubSectorIDOfCreatureAttack` (for groupless),
  `gfCantRetreatInPBI`. Downstream code keeps reading globals — one writer now.
- `TryActivateNextBattle()` guards, in order — return without activating if ANY holds:
  1. `gPendingBattles` empty.
  2. `gfPreBattleInterfaceActive`.
  3. `fDisableMapInterfaceDueToBattle` (merc quote playing).
  4. `gfWorldLoaded && gTacticalStatus.fEnemyInSector` (a tactical battle is running).
  5. An armed trigger is outstanding (step 3's `gPrebattleTriggerGroups` non-empty) AND the picked
     battle is not `fForcedAutoResolve` — a tactical-capable battle must not jump a battle whose
     box the player hasn't acknowledged. (Before step 3 lands, guard 5 reads the master
     equivalent: `gpInitPrebattleGroup != NULL`.)
  6. `guiCurrentScreen == GAME_SCREEN` (or the `JA2BETAVERSION` AIVIEWER case): set
     `gfEnteringMapScreen = TRUE; gfEnteringMapScreenToEnterPreBattleInterface = TRUE;` and return —
     the battle STAYS in the queue; nothing is copied to globals. This is what fixes the
     single-slot deferral.
  Pick rule: first entry with `fForcedAutoResolve`, else front. If the picked battle has
  `ubGroupID != 0` and `GetGroup()` returns NULL, drop it and pick again.
- Call sites for `TryActivateNextBattle()`: end of enqueue, and once per frame from
  `HandlePreBattleInterfaceStates` (replacing the drain the broken diff added there).

*Check:* single-battle cases behave exactly as master: plain merc encounter PBI, creature attack,
bloodcat ambush. Maintainer smoke-run. All four apps build.

### 2b. Deferral through the queue

In `HandlePreBattleInterfaceStates`, the replay branch
(`gfEnteringMapScreenToEnterPreBattleInterface && !gfEnteringMapScreen`) reduces to: clear the
flag, `TryActivateNextBattle()`. Delete the `gfUsePersistantPBI` consultation and the
`InitPreBattleInterface(gpBattleGroup, TRUE)` replay — the queue front already holds everything.
Remove now-dead writes of `gfUsePersistantPBI` in `InitPreBattleInterface`'s deferral block.
Do NOT yet remove the writes in `strategicmap.cpp` / `Strategic Movement.cpp` (2d handles the one
that matters; the rest become dead and are cleaned in 2e).

*Check (the step-1 scenario, partial):* trigger a battle while in tactical screen → exit to
mapscreen → correct PBI appears. Then two battles in one tick while in tactical → both survive
(order per pick rule). This is the check that the broken diff failed.

### 2c. Groupless transfer-to-autoresolve site

In `Strategic/strategicmap.cpp` (landmark: comment "We have militia and enemies and no mercs!
Let's finish this battle in autoresolve.", currently sets `gfUsePersistantPBI = FALSE` +
`gubPBSectorX/Y/Z` + `gfAutomaticallyStartAutoResolve = TRUE`): replace the
`gfUsePersistantPBI = FALSE` hack with enqueuing a `BATTLE{ubGroupID=0, sector, fPersistant=FALSE,
fForcedAutoResolve=TRUE}`. Leave `gfAutomaticallyStartAutoResolve` and
`gfTransferTacticalOppositionToAutoResolve` alone. STOP-AND-ASK with the surrounding code quoted
if the flag choreography there differs from this description.

*Check:* mercs retreat from a sector leaving militia vs enemies → autoresolve comes up for the
right sector.

### 2d. Forced-autoresolve marking

`gfDelayAutoResolveStart` (master) is the one-slot flag that caused the original reported bug. It
becomes `BATTLE::fForcedAutoResolve`, set at the arming sites that today set the flag:
`PrepareForPreBattleInterface` militia branch and `CheckConditionsForBattle` /
`CheckCombatInSectorDueToUnusualEnemyArrival` no-combat-able-merc branches (both in
`Strategic/Strategic Movement.cpp`). Since arming happens before `InitPreBattleInterface` runs
(message box in between), carry it as a module-level `std::set<UINT8>` of sectors exactly like the
broken diff's `gForcedAutoResolveSectors` — that part of the idea was sound — consumed into the
snapshot at enqueue time and erased at activation. In `HandlePreBattleInterfaceStates`, the branch
that reads `gfDelayAutoResolveStart && gfPreBattleInterfaceActive` instead checks the ACTIVE
battle's `fForcedAutoResolve` (keep the active `BATTLE` in a one-slot
`static BATTLE gActiveBattle` written by `ActivateBattle`).

*Check:* THE ORIGINAL BUG. Repro save: Drassen B13 box → OK → Drassen autoresolve PBI → resolve →
C8-C9 PBI with retreat legal (order per step-1 maintainer answer). This step is not done until the
maintainer sees exactly that.

### 2e. Sweep

Delete `gfDelayAutoResolveStart` and any now-unreferenced `gfUsePersistantPBI` writes
(readers should be zero after 2b — verify by grep; if a reader remains, STOP-AND-ASK).
`RemoveAllGroups` (`Strategic Movement.cpp`) clears the queue + forced-autoresolve set.

*Check:* grep shows no references; new game + load game both start with empty queue; four builds.

## Step 3 — one trigger per battle, IDs through the dialogue queue

These re-land the parts of the parked `opus-attempt` branch that were sound. Consult that branch's
diff for reference, but re-derive each edit against the step-1 trace.

### 3a. `NotifyPlayerOfInvasionByEnemyForces` returns BOOLEAN

(`Strategic/Map Screen Interface.cpp` + `.h`.) Return TRUE only when a message box with a non-NULL
callback went up (mine/town cases); the wilderness `ScreenMsg` case returns FALSE. Callers in
`CheckConditionsForBattle` and `CheckCombatInSectorDueToUnusualEnemyArrival`: pass the callback
only when not triggering directly, and trigger directly when the function returns FALSE — exactly
one trigger per battle. The opus-attempt version of this edit was correct; reuse it.

*Check:* SAM-site / militia-in-town invasion produces ONE PBI trigger (was two on master);
wilderness invasion still triggers.

### 3b. Armed-trigger FIFO

`gpInitPrebattleGroup` (single `GROUP*`) → `std::queue<UINT8> gPrebattleTriggerGroups` of group
IDs, pushed at the arming sites (bloodcat notify, no-merc invasion branches — including the
groupless `push(0)` for `CheckCombatInSectorDueToUnusualEnemyArrival`), popped in
`TriggerPrebattleInterface`. Guard 5 of `TryActivateNextBattle` now reads this queue.
Also `RemoveAllGroups` clears it.

### 3c. Kill the `GROUP*`-through-`UINT32` casts

- `TriggerPrebattleInterface` passes the popped `ubGroupID` (a `UINT8`, zero-extended) as
  `uiSpecialEventData` — not a pointer.
- `Tactical/Dialogue Control.cpp`, `DIALOGUE_SPECIAL_EVENT_TRIGGERPREBATTLEINTERFACE` block:
  Flugente already left the correct line commented out directly below his comment —
  `GROUP* pGroup = GetGroup( (UINT8)QItem.uiSpecialEventData );` — enable that form; NULL group is
  fine (`InitPreBattleInterface` handles groupless).
- Merc-quote path: `PrepareForPreBattleInterface` stores the group in
  `gpCurrentTalkingFace->uiUserData1`; store `ubGroupID` instead, and the
  `FACE_TRIGGER_PREBATTLE_INT` consumer in `Dialogue Control.cpp` resolves via `GetGroup()`.
  Before editing: grep other readers of `uiUserData1` on faces; if any consumer other than the
  `FACE_TRIGGER_PREBATTLE_INT` block reads it, STOP-AND-ASK.

### Landed (3a, 3b, 3c trigger half) — findings for step 2

`gpInitPrebattleGroup` is gone. `gPrebattleTriggerGroups` is pushed only where a trigger is actually
armed: the bloodcat branch and the `!fCombatAbleMerc` branch of both `CheckConditionsForBattle` and
`CheckCombatInSectorDueToUnusualEnemyArrival` (the latter pushes 0). Merc-quote battles no longer
touch it, which is what makes it usable as guard 5.

Two things found while editing that step 2 has to handle:

- **A group of only-unconscious mercs arms twice.** `CheckConditionsForBattle:1269` sets
  `pPlayerDialogGroup = pGroup` unconditionally for `OUR_TEAM`, so when every merc is below OKLIFE
  the `!fCombatAbleMerc` branch raises its box *and* execution falls through to
  `PrepareForPreBattleInterface`, whose `ubNumMercs == 0` else-branch calls `InitPreBattleInterface`
  inline. Today the second one is absorbed by `if( gfPreBattleInterfaceActive ) return;`. Once
  `InitPreBattleInterface` enqueues instead of dropping, that becomes **two BATTLE records for one
  battle**. Dedupe at enqueue: drop a push whose sector matches an entry already queued or active.
- **`CheckCombatInSectorDueToUnusualEnemyArrival` has no sector of its own.** Its trigger now carries
  group 0, so `InitPreBattleInterface( NULL, TRUE )` derives the sector from
  `gubSectorIDOfCreatureAttack` — stale unless a creature attack happened in the same sector. On
  master it instead passed whatever stale `GROUP*` a previous battle had left in
  `gpInitPrebattleGroup` (`:6199` is commented out), so this is not a regression, but it stays wrong
  until `BATTLE::ubSectorID` carries it. Do not ship step 2 without that field populated from `sX/sY`
  at this site.

*Check:* all three trigger paths (quote, box, direct) raise the correct PBI. Then the lifetime
test: maintainer breakpoints, destroy the armed group (`RemovePGroup`) while its box is up →
OK → no crash, battle skipped, next pending battle (if any) fires.

## Step 4 — persistence in `ubFiller`

STOP-AND-ASK before editing `Ja2/SaveLoadGame.cpp`: show the maintainer this layout and the exact
splice points first. That file's read-order arithmetic punishes mistakes.

- Struct: `GENERAL_SAVE_INFO.ubFiller[265]` — written and read unconditionally in the
  `NEW_GENERAL_SAVE_INFO_DATA` branch (landmark: `sGeneralInfo.ubFiller` in `SaveGeneralInfo` /
  `LoadGeneralInfo`). Verify `sGeneralInfo` is zeroed before fill on the save side; if not,
  zero the unused tail explicitly.
- Layout at the FRONT of `ubFiller`:
  - byte 0: pending-battle count, capped 16 (drop overflow, it cannot legitimately happen).
  - then per battle, 7 bytes: `ubGroupID, ubSectorID, bSectorZ, ubExplicitEncounterCode,
    ubEncounterCode, flags` (bit0 fPersistant, bit1 fForcedAutoResolve, bit2 fCantRetreat) + 1
    reserved. Max 1+16*7 = 113 of 265 bytes.
- Old saves: bytes are zero → count 0 → empty queue. No version bump, old exes ignore the bytes.
- Serialize the queue only. Battles waiting on a message box are still pre-`InitPreBattleInterface`
  and re-arm from strategic state on their own; the active PBI's fields
  (`fPersistantPBI`, explicit code) are already saved today — untouched.
- On load: populate `gPendingBattles` after groups are loaded; entries whose `ubGroupID` no longer
  resolves are dropped at pick time anyway.

*Check:* queue two merc battles, enter tactical for the first, save mid-fight, reload → finish →
second battle fires. Load a pre-change save → nothing pending, campaign unaffected. Save made by
new exe loads in it again (round-trip).

## Step 5 — acceptance sweep (all on maintainer's saves)

1. Original repro: B13 Drassen forced autoresolve AND C8-C9 merc PBI both happen, retreat legal
   on the merc one, order per step-1 ruling.
2. Two merc battles same tick while player in tactical → both survive deferral.
3. Armed group destroyed while box up → skip, no crash.
4. Save/reload mid-tactical with pending battle → pending battle fires (step 4 check).
5. Old savegame loads clean.
6. Unchanged behavior: creature attack, bloodcat ambush/lair, tactical traversal, player-clicked
   non-persistent PBI, same-sector simultaneous-arrival prompt.
7. All four applications build; `JA2UB` smoke-run if maintainer has a UB save.

## Appendix — parked full-dispatcher scope

Maintainer chose against it (risk). Kept for the record: real state machine
(PENDING/ARMED/DEFERRED/PBI_ACTIVE/AUTORESOLVE) replacing the deferral/handoff flags, then
migration of ~300 reader sites across 25 files (incl. `Multiplayer/client.cpp`) to
`ActiveBattle()` accessors, deleting the globals one per commit, `gpBattleGroup` last. The
middle-scope `BATTLE` record above is forward-compatible with it: the full version grows a state
field and the readers move; nothing here has to be undone.
