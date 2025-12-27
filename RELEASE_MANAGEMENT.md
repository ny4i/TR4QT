# TR4QT Release Management

This document describes the release and build management process for TR4QT, including how to create releases, beta versions, and test builds.

## Quick Start / TL;DR

**Just want to test something with others?**
```bash
git tag -a test-my-feature -m "Test build"
git push origin test-my-feature
# Download from GitHub Actions → Artifacts (expires in 30 days)
```

**Ready to release?**
```bash
# 1. Update version in src/core/Constants.h
# 2. Commit and push
git tag -a v2.91.0 -m "Release v2.91.0 - Description"
git push origin v2.91.0
# 3. Go to GitHub Releases, review draft, publish
```

**Tag Patterns:**
- `v2.91.0` → Draft release (you publish when ready)
- `v2.91.0-beta1` → Public pre-release (for testing)
- `v2.91.0-rc1` → Release candidate (final testing)
- `test-feature` → Build artifacts only (no release)

**All builds include Windows ZIP + macOS DMG and take ~15-20 minutes.**

---

## Overview

TR4QT uses a tag-based build system powered by GitHub Actions. Different tag patterns trigger different types of builds and releases:

- **Production releases** (`v*.*.*`) - Draft releases for stable versions
- **Beta releases** (`v*.*.*-beta*`) - Pre-releases for public testing
- **Release candidates** (`v*.*.*-rc*`) - Final testing before production
- **Test builds** (`test-*`) - Temporary builds for testing (no release created)

## Tag Patterns

### Production Releases: `v*.*.*`

**Example:** `v2.90.1`, `v2.91.0`

**Behavior:**
- Builds Windows (MinGW) and macOS versions
- Runs test suite on both platforms
- Creates a **draft GitHub Release**
- Uploads Windows ZIP and macOS DMG to the draft release
- Generates release notes automatically

**When to use:**
- Stable versions ready for public release
- Bug fix releases
- Major feature releases

**Usage:**
```bash
# Update version in src/core/Constants.h first
git add src/core/Constants.h
git commit -m "Bump version to 2.91.0"
git push origin master

# Create and push tag
git tag -a v2.91.0 -m "Release v2.91.0 - Add band needs display widget"
git push origin v2.91.0
```

**Next steps:**
1. Go to GitHub → Releases → Drafts
2. Review the automatically generated release notes
3. Edit the release description as needed
4. Click "Publish release" when ready

---

### Beta Releases: `v*.*.*-beta*`

**Example:** `v2.91.0-beta1`, `v3.0.0-beta2`

**Behavior:**
- Builds Windows and macOS versions
- Runs test suite
- Creates a **public pre-release** (not draft)
- Marked with yellow "Pre-release" badge on GitHub
- Immediately visible to users checking for updates

**When to use:**
- Feature testing with wider audience
- Gathering feedback before final release
- Breaking changes that need validation

**Usage:**
```bash
git tag -a v2.91.0-beta1 -m "Beta 1 for v2.91.0 - Band needs display"
git push origin v2.91.0-beta1
```

**Versioning convention:**
- `v2.91.0-beta1` - First beta
- `v2.91.0-beta2` - Second beta (after fixes)
- `v2.91.0-beta3` - Continue numbering

---

### Release Candidates: `v*.*.*-rc*`

**Example:** `v2.91.0-rc1`, `v3.0.0-rc2`

**Behavior:**
- Builds Windows and macOS versions
- Runs test suite
- Creates a **public pre-release** (not draft)
- Code is expected to be stable and feature-complete
- Only critical bug fixes should follow

**When to use:**
- Final testing before production release
- Code freeze - no new features
- Waiting for final QA approval

**Usage:**
```bash
git tag -a v2.91.0-rc1 -m "Release candidate 1 for v2.91.0"
git push origin v2.91.0-rc1
```

**Typical flow:**
1. Finish all features → `v2.91.0-beta1`
2. Fix beta issues → `v2.91.0-beta2`
3. Code freeze → `v2.91.0-rc1`
4. Final testing → `v2.91.0-rc2` (if needed)
5. Production → `v2.91.0`

---

### Test Builds: `test-*`

**Example:** `test-needs-widget`, `test-2024-12-27`, `test-macos-fix`

**Behavior:**
- Builds Windows and macOS versions
- Runs test suite
- **Does NOT create a GitHub Release**
- Uploads artifacts to GitHub Actions (30-day retention)
- Perfect for quick testing without cluttering releases page

**When to use:**
- Testing new features before committing to a release
- Sharing builds with specific testers
- CI/CD verification
- Temporary builds that shouldn't be permanent

**Usage:**
```bash
# Create a test build
git tag -a test-needs-widget -m "Test build for band needs display widget"
git push origin test-needs-widget
```

**Accessing test artifacts:**
1. Go to https://github.com/ny4i/TR4QT/actions
2. Find the workflow run for your test tag
3. Wait for the build to complete (green checkmark)
4. Scroll to the "Artifacts" section at the bottom
5. Download `test-build-test-needs-widget`
6. Extract the ZIP to find Windows and macOS builds

**Sharing with testers:**
- Copy the Actions run URL and send to testers
- They can download artifacts directly (requires GitHub account)
- Artifacts expire after 30 days

**Deleting test tags:**
```bash
# After testing is complete, clean up test tags
git tag -d test-needs-widget
git push origin :refs/tags/test-needs-widget
```

---

## Complete Release Workflow

### 1. Development Phase
```bash
# Work on features in master branch
git checkout master
# ... make changes ...
git add .
git commit -m "Add new feature"
git push origin master
```

### 2. Internal Testing (Test Build)
```bash
# Create test build for quick verification
git tag -a test-feature-name -m "Test build for feature XYZ"
git push origin test-feature-name

# Download from Actions, test locally
# Fix any issues, push fixes
# Create another test build if needed
```

### 3. Beta Testing (Public Feedback)
```bash
# Update version to beta
# Edit src/core/Constants.h: APP_VERSION = "2.91.0-beta1"
git add src/core/Constants.h
git commit -m "Prepare v2.91.0-beta1"
git push origin master

git tag -a v2.91.0-beta1 -m "Beta 1 for v2.91.0"
git push origin v2.91.0-beta1

# Announce beta in discussions/social media
# Gather feedback, fix issues
# Create beta2 if needed
```

### 4. Release Candidate (Final QA)
```bash
# Update version to rc
# Edit src/core/Constants.h: APP_VERSION = "2.91.0-rc1"
git add src/core/Constants.h
git commit -m "Prepare v2.91.0-rc1"
git push origin master

git tag -a v2.91.0-rc1 -m "Release candidate 1 for v2.91.0"
git push origin v2.91.0-rc1

# Final testing by core team
# Fix only critical bugs
```

### 5. Production Release
```bash
# Update version to final
# Edit src/core/Constants.h: APP_VERSION = "2.91.0"
git add src/core/Constants.h
git commit -m "Release v2.91.0"
git push origin master

git tag -a v2.91.0 -m "Release v2.91.0 - Band needs display widget"
git push origin v2.91.0

# Go to GitHub Releases, review draft, publish
```

---

## Version Numbering

TR4QT follows semantic versioning: `MAJOR.MINOR.PATCH`

- **MAJOR** (2.x.x) - Breaking changes, major rewrites
- **MINOR** (x.91.x) - New features, backwards compatible
- **PATCH** (x.x.1) - Bug fixes, minor improvements

**Examples:**
- `v2.90.1` → `v2.90.2` - Bug fix
- `v2.90.2` → `v2.91.0` - New feature (band needs display)
- `v2.91.0` → `v3.0.0` - Major rewrite or breaking change

---

## GitHub Actions Workflow

### Automated Build Process

When you push a tag, GitHub Actions automatically:

1. **Checkout code** from the tagged commit
2. **Build Windows version**
   - Install Qt 6.7.2 and MinGW
   - Download Hamlib 4.5.5
   - Build with CMake
   - Run test suite (continue-on-error)
   - Deploy Qt dependencies with windeployqt
   - Upload artifact
3. **Build macOS version**
   - Install Qt 6.7.2 and Hamlib via Homebrew
   - Build with CMake
   - Run test suite (continue-on-error)
   - Create DMG with macdeployqt
   - Upload artifact
4. **Create release** (for `v*` tags) or **Upload artifacts** (for `test-*` tags)

### Build Time

Typical build times:
- Windows: ~8-10 minutes
- macOS: ~6-8 minutes
- **Total: ~15-20 minutes**

### Monitoring Builds

View build status: https://github.com/ny4i/TR4QT/actions

**Build statuses:**
- 🟡 Yellow dot - In progress
- ✅ Green checkmark - Success
- ❌ Red X - Failed

**If a build fails:**
1. Click on the failed workflow run
2. Expand the failed step to see error logs
3. Fix the issue locally
4. Delete the failed tag: `git push origin :refs/tags/v2.91.0`
5. Create a new tag after fixing

---

## Best Practices

### DO:
✅ Update `APP_VERSION` in `src/core/Constants.h` before tagging
✅ Test locally before creating release tags
✅ Use test builds for quick iterations
✅ Write meaningful tag messages
✅ Delete old test tags after testing
✅ Use semantic versioning
✅ Create release candidates for major versions
✅ Test on both Windows and macOS before production release

### DON'T:
❌ Don't push `v*` tags for unfinished features
❌ Don't reuse tag names (delete and recreate is okay during testing)
❌ Don't skip version updates in Constants.h
❌ Don't publish draft releases without review
❌ Don't create releases for minor commits (use test builds)
❌ Don't forget to clean up old test tags

---

## Troubleshooting

### "403 Forbidden" when creating release
- Workflow needs `contents: write` permission
- Already configured in `.github/workflows/build.yml`
- Only occurs if workflow file is modified incorrectly

### Build fails with "test_qso_persistence failed"
- This test is disabled in CI (Qt resource loading issue)
- Tests pass locally but fail in GitHub Actions
- This is expected and won't block releases

### Tag already exists
```bash
# Delete local and remote tag
git tag -d v2.91.0
git push origin :refs/tags/v2.91.0

# Recreate
git tag -a v2.91.0 -m "Release v2.91.0"
git push origin v2.91.0
```

### Can't find test build artifacts
- Wait for workflow to complete (green checkmark)
- Go to Actions → Click on workflow run
- Scroll to bottom → "Artifacts" section
- Test artifacts expire after 30 days

### macOS build shows linker warnings
- Expected: Building for macOS 11.0 but linking with newer libraries
- Not a problem: App will run on macOS 11.0+ (Big Sur through Sequoia)
- Ensures backward compatibility with older macOS versions

---

## Quick Reference

| Task | Command |
|------|---------|
| Production release | `git tag -a v2.91.0 -m "Release 2.91.0"` |
| Beta release | `git tag -a v2.91.0-beta1 -m "Beta 1"` |
| Release candidate | `git tag -a v2.91.0-rc1 -m "RC 1"` |
| Test build | `git tag -a test-feature -m "Test feature"` |
| Delete tag locally | `git tag -d v2.91.0` |
| Delete tag remotely | `git push origin :refs/tags/v2.91.0` |
| List all tags | `git tag -l` |
| View tag details | `git show v2.91.0` |

---

## Additional Resources

- GitHub Actions Workflow: `.github/workflows/build.yml`
- Version History: `src/core/Constants.h`
- CI Status: https://github.com/ny4i/TR4QT/actions
- Releases: https://github.com/ny4i/TR4QT/releases
