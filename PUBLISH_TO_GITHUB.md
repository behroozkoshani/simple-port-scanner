# Publish as a standalone GitHub repository

This project is intended to be its **own GitHub repository**, `behroozkoshani/simple-port-scanner`, rather than a subfolder of the GitHub Pages website or the Metasploit lab.

## Windows (PowerShell)

1. Extract the ZIP file and open PowerShell in the extracted `simple-port-scanner` folder.
2. Install [Git](https://git-scm.com/downloads) and [GitHub CLI](https://cli.github.com/) if needed.
3. Run `gh auth login` to sign in to **your** GitHub account.
4. Run `powershell -ExecutionPolicy Bypass -File .\publish.ps1`.

The script verifies the signed-in account, initializes a local Git repository, commits the source/docs/screenshots, creates a **new public standalone repository**, and pushes to `main`. It refuses to proceed if the target repo already exists, so it will not overwrite unrelated work.

## Linux / macOS / WSL

1. Extract the ZIP file and open a terminal in the extracted `simple-port-scanner` folder.
2. Install Git and GitHub CLI; run `gh auth login`.
3. Run `bash ./publish.sh`.

## After publishing

Open `https://github.com/behroozkoshani/simple-port-scanner` and confirm the README images render, source and tests are present, and the Actions workflow passes. You can then link the **standalone** repo on your portfolio and resume.

If you prefer GitHub's website: create a new empty public repository at `https://github.com/new` with the name `simple-port-scanner` (no README), then from this extracted folder run `git init -b main`, `git add .`, `git commit -m "Initial project"`, `git remote add origin https://github.com/behroozkoshani/simple-port-scanner.git`, and `git push -u origin main` with your account authenticated.

**Do not place the project inside `behroozkoshani.github.io` or `metasploit-attack-simulation-lab`.**
