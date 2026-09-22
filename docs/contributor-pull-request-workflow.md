# Contributor Pull Request Workflow

This workflow defines how firmware changes move from a contributor branch into the MeowKit-S3 production repository.

## 1. Scope the change

Every change starts with a single, testable problem. The issue or pull request must describe:

- the affected hardware and firmware version;
- exact reproduction steps;
- expected and actual behavior;
- user-visible impact and risk;
- source attribution when code is adapted from another project.

Keep unrelated fixes in separate pull requests. Small, focused changes are easier to review, test, revert, and credit.

## 2. Prepare the branch

Create a descriptive branch from the latest `main`, for example:

```text
fix/joystick-navigation-reboot
feat/web-firmware-update
docs/advanced-settings
```

Before editing, confirm the tree is clean and update the branch from `origin/main`. Never overwrite another contributor's uncommitted work.

The required library sources are vendored under `lib/`. A normal clone contains everything needed to build; no submodule initialization is required.

## 3. Implement the firmware change

Follow the existing architecture and product interaction model.

- Keep hardware ownership clear. Stop radio, USB, audio, timers, and background tasks in the application's close path.
- Avoid starting a second LVGL screen transition while another transition is active.
- Keep physical input deterministic. One action must produce one state transition.
- Preserve persistent settings and flush them before restart or shutdown.
- Do not commit build directories, local credentials, editor state, or generated release packages unless the repository explicitly tracks them.

Comments should explain constraints and failure modes, not restate the code.

## 4. Record the change

Update `CHANGELOG.md` under **Unreleased**. Record:

- the user-visible result;
- significant internal behavior changes;
- contributor and upstream credit;
- validation completed;
- validation still pending.

Documentation must not claim that hardware testing passed when only compilation was performed.

## 5. Validate locally

The minimum software gate is:

```powershell
platformio run
git diff --check
```

For navigation and UI changes, test on MeowKit hardware:

1. Boot the device and confirm the Home screen is stable.
2. Rapidly reverse the joystick during every animated Home transition.
3. Repeat navigation for at least 100 transitions without a reboot or frozen input.
4. Open Settings, Wi-Fi, Advanced, Clock, SD Files, and all affected apps.
5. Verify A, short B, long B, joystick directions, and touch behavior.
6. Check Serial output for reset reasons, assertions, watchdog events, and falling heap.
7. Power-cycle once and repeat the critical path.

Record the board revision, firmware commit, test duration, result, and any serial logs.

## 6. Push and open the pull request

Use a clear commit message:

```text
fix(navigation): prevent overlapping screen transitions
```

The pull request must include:

- problem and root cause;
- implementation summary;
- files and subsystems affected;
- risks and rollback plan;
- build result and hardware test evidence;
- screenshots or serial logs when they clarify the behavior;
- attribution and license compatibility for adapted code.

## 7. Review

A maintainer reviews:

- correctness and lifecycle safety;
- concurrency, timing, memory, and peripheral cleanup;
- input and screen-state edge cases;
- persistence and backward compatibility;
- documentation, attribution, and license obligations;
- accidental unrelated changes.

Requested changes are resolved in the contributor branch. Re-run the complete validation set after each material revision.

## 8. Merge gate

A pull request can enter `main` only when:

- review comments are resolved;
- required checks pass;
- the release build succeeds;
- hardware tests for the affected path pass;
- documentation is current;
- the branch has no unresolved conflicts;
- contributor authorship is preserved.

Use a merge commit when preserving the contributor's commit history is valuable. Use squash merge for noisy development history while retaining the contributor in the final commit authorship and pull request record.

## 9. Post-merge verification

After merge:

1. Build `main` from a clean checkout.
2. Repeat the critical hardware smoke test.
3. Confirm the pull request is linked from the changelog.
4. Publish release artifacts only from the tested commit.
5. Tag the exact release commit and attach checksums.
6. Keep the previous stable firmware available for rollback.

If a regression appears, revert the merge commit first, restore the last stable release, and diagnose on a new branch.

## PR #29 integration record

- Pull request: [#29 — Prevent reboot during screen transitions](https://github.com/mingolucky/meowkit-s3-firmware/pull/29)
- Contributor: [@warengonzaga](https://github.com/warengonzaga)
- Upstream credit: [@janud](https://github.com/janud)
- Reviewed files: Launcher navigation, shared LVGL screen helper, Wi-Fi screen, Settings screen
- Software build: Passed for `esp32s3box`
- Hardware regression test: Pending
- Merge requirement: complete the rapid-navigation and app-exit hardware tests above before production release
