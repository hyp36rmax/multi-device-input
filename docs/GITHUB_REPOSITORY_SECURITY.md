# GitHub repository security policy

This document records the intended repository administration policy for
`hyp36rmax/multi-device-input`. It does not change application, Multi-Input,
HYP36R Force, telemetry, save, or release runtime behavior.

## Repository model

- Owner: personal account `hyp36rmax`
- Visibility: public fork of `emoose/OutRun2006Tweaks`
- Default production branch: `master`
- Active development branch: `multi-device-input`
- Other retained branch: `r2-pre-cleanup-test`
- Current product release tag convention: semantic version tags such as
  `v1.0.0` and the planned `v1.5.0`
- No release branches and no established `release/*` convention currently exist

## Ruleset policy

### All branch history

Every branch is protected by an active branch ruleset with:

- restrict deletions;
- block force pushes;
- no global update restriction;
- no global pull-request requirement;
- no global branch lock;
- no routine bypass actor.

This preserves ordinary forward pushes, new development branches and the
existing Codex workflow while protecting branch history.

### Default branch

`master` is additionally protected by an active ruleset requiring:

- changes through a pull request;
- the existing `build (Win32, Release)` check;
- conversation resolution;
- no deletion;
- no force push.

No approving-review count is required while the project remains a single-
maintainer repository. This avoids a merge deadlock without permitting direct
unreviewed updates to the production branch.

### Release branches

No release-branch ruleset exists because the repository does not currently use
release branches or an established release-branch naming convention. If that
workflow is adopted, define and review the naming convention before adding a
PR-and-CI ruleset.

### Release tags

An active tag ruleset targets `v*`. It permits creation of a new version tag but
restricts deletion and updates, preventing an existing release tag from being
moved or rewritten. The historical `multi-device-v0.1.0` tag is outside the
current product convention and is not retroactively included.

No routine automation bypass is required. The current release-notes workflow
edits release text for `multi-device-v0.1.0`; it does not create, move, or delete
tags.

## Actions permissions

The repository default `GITHUB_TOKEN` permission is read-only for repository
contents and packages. GitHub Actions cannot create or approve pull requests.

The build workflow explicitly uses `contents: read` and cannot push commits,
branches or tags, merge pull requests, or create releases.

The release-notes synchronization workflow is the only write-capable workflow.
Its `sync` job has `contents: write` solely because `gh release edit` updates the
notes of the existing `multi-device-v0.1.0` release. The permission is scoped to
that job and the workflow runs only when its release-note source or workflow file
changes on `multi-device-input`.

Neither workflow uses `pull_request_target`, `workflow_run`, repository secrets,
OIDC, package writes, Actions writes, check writes, pull-request writes, branch
pushes, tag operations, automated merges, or contributor-controlled code with a
write token.

Fork pull-request workflows require approval for first-time contributors. The
repository currently permits all marketplace actions; requiring full commit-SHA
pinning would break the current workflows because their Actions references use
version tags. That setting must not be enabled until those references have been
reviewed and pinned deliberately.

## Native security features

The intended low-risk baseline is:

- dependency graph enabled;
- Dependabot alerts enabled;
- Dependabot security updates enabled;
- private vulnerability reporting enabled;
- secret scanning enabled;
- secret push protection enabled.

Dependabot version updates are not enabled without a reviewed update policy.
CodeQL is not a mandatory merge gate until a stable C++ analysis configuration
has been proven against this Windows-focused project.

## Security audit automation boundary

A scheduled drift detector is desirable, but the normal repository
`GITHUB_TOKEN` does not have repository-administration read access to inspect
rulesets and branch-protection settings. A complete detector would require a
fine-grained token or GitHub App with read-only Administration access.

No credential is added for this milestone. A future audit job should:

1. run read-only on a weekly schedule and manual dispatch;
2. read rulesets, default-branch protection and Actions permission settings;
3. compare them with this document;
4. fail and create a visible run summary on drift;
5. never modify settings or receive contents/write access.

Prefer a dedicated GitHub App or narrowly scoped fine-grained token. Never use a
personal classic token or a credential with repository-content write access for
this audit.

## Recovery and emergency administration

Rulesets have no routine bypass actors. If recovery is necessary, the repository
owner can open **Settings → Rules → Rulesets**, deliberately disable or edit the
specific rule, perform the minimum recovery action, and immediately restore
active enforcement. Record why the exception was necessary and verify the
ruleset afterward.

Do not validate protections by deleting or force-pushing an important branch or
moving a release tag. Use GitHub's active ruleset state and a disposable branch
only when a non-destructive configuration check is insufficient.

## Future leaderboard boundary

The public client must never contain production leaderboard API secrets,
database credentials, private signing keys, moderation or administration
credentials, deployment secrets, or private server validation logic that is not
required by the shipped client.

Any leaderboard server belongs in a separate private repository and separately
managed service. This public repository may eventually contain only a documented
public protocol and the minimum client needed to communicate with it.
