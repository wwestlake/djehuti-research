# Contributing

This independent repository uses `master` as its primary branch. The initial repository-readiness/source publication is Work Order 2a. Subsequent implementation uses a short-lived branch for one owner-authorized change and a pull request to `master`. The owner reviews and authorizes merges; an agent does not merge its own unapproved PR. No branch protections are installed without approval.

Before editing, read `AGENTS.md` and the current owner-issued work order. Inspect Git status, preserve unrelated work and coordinate a single writable owner. Commit only reviewed files. No force-push or history rewrite without explicit owner authorization.

Run checks appropriate to the change. Application work normally uses Windows Debug, one core, the existing build directory and focused functional verification. Documentation-only edits do not require an application build. Redirect logs to `work/`, await completion and report actual results. Close every test application and confirm exit.

Never commit credentials, local environment/preset files, user research, caches, build outputs or diagnostic symbols. An ignore rule does not remove something already in Git history: review both the proposed tree and every commit that will be published. Keep private work-order locations in the ignored local configuration.

Do not modify reference repositories or the installed LLVM toolchain as an incidental dependency fix. No website deployment, release workflow or binary upload is implied by a source-code PR.
