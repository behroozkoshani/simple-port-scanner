#!/usr/bin/env bash
# Publish as a new standalone GitHub repo. Does not modify other repositories.
set -euo pipefail
cd "$(dirname "$0")"
owner="behroozkoshani"
repo="simple-port-scanner"
command -v gh >/dev/null || { echo 'Install GitHub CLI: https://cli.github.com/' >&2; exit 1; }
command -v git >/dev/null || { echo 'Install Git: https://git-scm.com/' >&2; exit 1; }
gh auth status >/dev/null 2>&1 || { echo 'Sign in first: gh auth login' >&2; exit 1; }
user="$(gh api user --jq .login)"
[[ "$user" == "$owner" ]] || { echo "Wrong GitHub account: $user (expected $owner)" >&2; exit 1; }
if gh repo view "$owner/$repo" >/dev/null 2>&1; then
  echo "Repository $owner/$repo already exists. Refusing to overwrite anything." >&2; exit 1
fi
[[ ! -d .git ]] || { echo 'This folder already contains a Git repository; inspect it before publishing.' >&2; exit 1; }
[[ -n "$(git config user.email || true)" ]] || { echo 'Set your Git commit email: git config --global user.email "your-email"' >&2; exit 1; }
[[ -n "$(git config user.name || true)" ]] || git config --global user.name 'Behrooz Koshani'
git init -b main
git add README.md CMakeLists.txt LICENSE src scripts tests screenshots docs .github .gitignore PUBLISH_TO_GITHUB.md publish.sh publish.ps1
git commit -m "Add authorized C++ TCP connect scanner with verified local evidence"
gh repo create "$owner/$repo" --public --source . --remote origin --push
printf '\nPublished: https://github.com/%s/%s\n' "$owner" "$repo"
