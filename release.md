# Making a Release

Releases are created by pushing a version tag. This triggers a GitHub Actions workflow that cross-compiles Windows x64 binaries and publishes them as a release asset.

## Steps

**1. Make sure the `master` branch is in a releasable state** — all changes committed and pushed.

**2. Create and push a version tag:**

```bash
git tag v1.2.3
git push origin v1.2.3
```

## What happens automatically

Pushing the tag triggers the workflow defined in `.github/workflows/release.yml`:

1. Checks out the tagged commit
2. Installs the MinGW-w64 cross-compiler on an Ubuntu runner
3. Builds `dymon_pbm.exe`, `dymon_srv.exe`, and `txt2pbm.exe` for Windows x64
4. Strips the binaries to reduce size
5. Packages them into `dymon-windows-x64.zip`
6. Creates a GitHub release for the tag and attaches the zip as a download asset

The release is visible under the **Releases** section of the GitHub repository.
