# Self-Hosted Runner Security for Public Repos

## The Risk

When using self-hosted GitHub Actions runners on a **public repository**, anyone can:

1. Fork your repo
2. Submit a PR with malicious workflow changes
3. If approved/merged, that code runs on YOUR server
4. Attacker could steal secrets, install malware, pivot to your network

## Mitigations (Implement Before Accepting External PRs)

### Option 1: Use Self-Hosted Only for Trusted Events (Recommended)

Update `.github/workflows/build.yml` to use GitHub-hosted runners for PRs:

```yaml
build-linux-x86_64:
  name: Linux (x86_64)
  # Use self-hosted for pushes/tags, GitHub-hosted for PRs
  runs-on: ${{ github.event_name == 'pull_request' && 'ubuntu-24.04' || fromJSON('["self-hosted", "linux", "X64"]') }}
```

### Option 2: Limit to Specific Branches

Only run on master and tags (skip PR builds entirely for Linux):

```yaml
build-linux-x86_64:
  name: Linux (x86_64)
  if: github.ref == 'refs/heads/master' || startsWith(github.ref, 'refs/tags/')
  runs-on: [self-hosted, linux, X64]
```

### Option 3: Require Approval for All External Contributors

GitHub Settings → Actions → General → "Require approval for all outside collaborators"

This requires you to manually approve workflow runs from forks before they execute.

### Option 4: Review Workflow Changes Carefully

Any PR that modifies `.github/workflows/` files should be scrutinized. Malicious code could:
- Exfiltrate secrets
- Install backdoors
- Access your local network
- Modify build artifacts

## Current Status

- **Repository**: TR4QT (public)
- **Self-hosted runner**: linux-ci-build (VM)
- **Risk level**: Low (single maintainer, no external PRs yet)
- **Action needed**: Implement mitigations before accepting first external PR

## References

- [GitHub Docs: Self-hosted runner security](https://docs.github.com/en/actions/hosting-your-own-runners/managing-self-hosted-runners/about-self-hosted-runners#self-hosted-runner-security)
- [GitHub Docs: Approving workflow runs from public forks](https://docs.github.com/en/actions/managing-workflow-runs/approving-workflow-runs-from-public-forks)
