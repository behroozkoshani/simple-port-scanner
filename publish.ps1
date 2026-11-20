# Publishes one NEW repository only; refuses to overwrite existing GitHub projects.
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$owner = 'behroozkoshani'
$repo = 'simple-port-scanner'
if (-not (Get-Command gh -ErrorAction SilentlyContinue)) { throw 'Install GitHub CLI from https://cli.github.com/' }
if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw 'Install Git from https://git-scm.com/' }
gh auth status *> $null
if ($LASTEXITCODE -ne 0) { throw 'Run gh auth login first.' }
$account = (gh api user --jq .login).Trim()
if ($account -ne $owner) { throw "Signed in as $account instead of $owner." }
try { gh repo view "$owner/$repo" *> $null } catch { $global:LASTEXITCODE = 1 }
if ($LASTEXITCODE -eq 0) { throw "Repository $owner/$repo already exists; refusing to overwrite it." }
if (Test-Path '.git') { throw 'This folder already has a Git repository. Inspect before publishing.' }
$email = git config user.email
if (-not $email) { throw 'Set your Git commit email first: git config --global user.email "your-email"' }
$name = git config user.name
if (-not $name) { git config --global user.name 'Behrooz Koshani' }
git init -b main
if ($LASTEXITCODE -ne 0) { throw 'git init failed.' }
git add README.md CMakeLists.txt LICENSE src scripts tests screenshots docs .github .gitignore PUBLISH_TO_GITHUB.md publish.sh publish.ps1
if ($LASTEXITCODE -ne 0) { throw 'git add failed.' }
git commit -m 'Add authorized C++ TCP connect scanner with verified local evidence'
if ($LASTEXITCODE -ne 0) { throw 'git commit failed.' }
gh repo create "$owner/$repo" --public --source . --remote origin --push
if ($LASTEXITCODE -ne 0) { throw 'GitHub repository creation or push failed.' }
Write-Host "Published: https://github.com/$owner/$repo"

