# Security policy

## Supported releases

Security fixes are considered for the current stable Multi Input release line.
Development builds and older releases may be useful for diagnosis, but are not
separately supported security branches.

## Reporting a vulnerability

Please use GitHub's private vulnerability reporting for this repository:

<https://github.com/hyp36rmax/multi-device-input/security/advisories/new>

Do not put credentials, private keys, exploitable security details, private
system information, or other sensitive findings in a public Issue, Discussion,
pull request, commit, or log attachment. If private vulnerability reporting is
unavailable, contact the repository owner through GitHub without publishing the
sensitive details. This project does not publish a private email address.

Include the affected release or commit, expected and observed behavior, impact,
and the smallest safe reproduction you can provide. Do not include game save
data, account credentials, or unrelated personal information.

## Repository integrity

Repository rules protect branches against deletion and force-push history
rewrites. The default production branch additionally requires pull requests and
the established Windows Release build before merge. Release tags matching `v*`
are protected against deletion and movement after creation.

Temporary emergency changes to these protections require deliberate repository-
owner administration. Routine CI and development automation are not bypass
actors.

## Credentials and future services

This public client repository must never contain production service secrets,
database credentials, private signing keys, moderation or administrator
credentials, deployment secrets, or private server-side validation logic that
does not need to ship with the client.

Any future leaderboard backend belongs in a separate private repository or
service. This repository should contain only the public client protocol and the
minimum client functionality required to communicate with that service.
