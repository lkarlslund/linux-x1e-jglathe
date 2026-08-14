# ThinkPad T14s X1E kernel packages

This repository builds four independently installable Arch Linux ARM kernels.
Each package has its own kernel release, initramfs, systemd-boot entry, module
directory, and DTB directory. Installing or upgrading one variant does not
replace the boot files or device trees of another variant.

## Variants

| Package | Base | Purpose |
| --- | --- | --- |
| `linux-x1e-jens71` | Jens `7.1.7-jg-1` | Stable recovery and daily-use baseline; PSR disabled in its boot entry |
| `linux-x1e-jens72` | Jens `7.2-rc6-jg-0` | Unmodified Jens 7.2 release-candidate integration |
| `linux-x1e-jens72-pdc` | Jens `7.2-rc6-jg-0` | Qualcomm PDC/SS3 v4 suspend series plus corrected USB PHY supplies |
| `linux-x1e-t14s-edge` | Mainline `7.2-rc7` snapshot | Bleeding-edge T14s branch with PDC/SS3, direct PSR SDP flushing, and corrected T14s USB PHY supplies |

All source revisions are immutable 40-character Git commit IDs in their
respective `PKGBUILD`. Updating a branch on GitHub does not silently alter a
package build.

The two integration branches are maintained at
[`lkarlslund/linux-x1e-t14s`](https://github.com/lkarlslund/linux-x1e-t14s):

- `t14s/jens-7.2rc6-pdc-v4`
- `t14s/mainline-edge`

The Jens PDC/SS3 branch carries Qualcomm's v4 series. It is experimental,
especially on firmware that leaves the PDC in secondary-controller mode.

## Build

Verify all package definitions and collision boundaries:

```bash
./scripts/verify-packages
```

Build one kernel:

```bash
./scripts/build-kernels linux-x1e-jens72-pdc
```

Build all four sequentially:

```bash
./scripts/build-kernels all
```

The local makepkg configuration controls `SRCDEST`, `PKGDEST`, and parallelism.
On this machine packages are written to `/home/lak/.cache/makepkg/pkg` and Git
sources are shared through `/home/lak/.cache/makepkg/src`.

## GitHub builds and releases

GitHub Actions builds packages natively on ARM64 and publishes them to the
rolling `packages` release. Commits do not start builds. Run the workflow
manually and select one variant or all variants. The `build_headers` input is
off by default; enable it only when matching headers packages are needed.

Every affected PKGBUILD must receive a new `pkgver` or `pkgrel`. Publishing
refuses to replace an existing package filename, which prevents a changed
binary from being presented as an old package version.

The rolling release retains unchanged packages, replaces the selected kernel
packages, and regenerates `linux-x1e-t14s.db` and `linux-x1e-t14s.files`. A
headers-disabled build also removes older headers for the selected variants.
Packages are currently unsigned, so the release is
suitable for direct download and local `pacman -U` installation. Do not enable
it as a pacman sync repository with signature checking disabled; package
signing should be configured before doing that.

Builds use a bounded 1.5 GB `ccache` per variant and retain makepkg's Git source
mirror in the repository's separate Actions cache allowance. The temporary
workflow artifacts used to join the matrix jobs are deleted after a successful
release, leaving the downloadable packages as release assets.

Release page:

<https://github.com/lkarlslund/linux-x1e-jglathe/releases/tag/packages>

Install kernel packages with `pacman -U`. Header packages are optional and can
be installed independently. For example:

```bash
sudo pacman -U /home/lak/.cache/makepkg/pkg/linux-x1e-jens72-pdc-*.pkg.tar.zst
```

Review the glob before confirming so that headers are only installed when
wanted.

## Collision-free boot layout

For a package named `$pkgbase`, installation creates:

```text
/boot/vmlinux-$pkgbase
/boot/vmlinuz-$pkgbase
/boot/initramfs-$pkgbase.img
/boot/dtbs/$pkgbase/x1e78100-lenovo-thinkpad-t14s*.dtb
/boot/loader/entries/$pkgbase.conf
/usr/lib/modules/<unique-kernel-release>/
```

The systemd-boot entry always references the DTB in that package's directory.
No package installs a DTB into a shared `/boot/dtbs/qcom` location.

The entries use the OLED EL2 device tree because that is the target hardware.
PKGBUILDs contain no machine-specific storage identifiers. During installation,
each package discovers the target system's root filesystem UUID and adds it to
its own loader entry. Hibernation arguments are intentionally omitted.

Installing these packages does not change the systemd-boot default. Keep the
known-good 7.1 entry selected as the default until another variant has passed
boot, suspend, resume, display, USB, and power-consumption testing.

## Updating a variant

1. Update or rebase the appropriate kernel branch.
2. Test the touched kernel objects and T14s DTB.
3. Push the branch to its remote.
4. Replace `_commit` and `pkgver` in only that variant's `PKGBUILD`.
5. Increment `pkgrel` when packaging changes without changing the kernel base.
6. Run `./scripts/verify-packages`, then build and install that variant.

The old one-package layout and unused historical camera patches are retained in
`legacy-patches/` for provenance; they are not applied to any current variant.
