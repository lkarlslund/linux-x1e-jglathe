# ThinkPad T14s X1E kernel packages

This repository builds two independently installable Arch Linux ARM kernels.
Each package has its own kernel release, initramfs, systemd-boot entry, module
directory, and DTB directory. Installing or upgrading one variant does not
replace the boot files or device trees of another variant.

## Variants

| Package | Base | Purpose |
| --- | --- | --- |
| `linux-x1e-jens725-pdc` | Jens `7.2.5-jg-0` | Stable daily-use kernel with the rc6 PDC/SS3 v4 series and diagnostics |
| `linux-x1e-jens73` | Jens `7.3.0-jg-1` | The single 7.3 variant: Jens's Snapdragon laptop integration, trimmed laptop module set, X1E EL2 s2idle/DRIPS work |

All source revisions are immutable 40-character Git commit IDs in their
respective `PKGBUILD`. Updating a branch on GitHub does not silently alter a
package build.

The retired `linux-x1e-jens71`, `linux-x1e-jens72` and `linux-x1e-jens72-pdc`
variants have been removed; their recipes remain in Git history, and the next
publish removes their release assets.

The historical integration branches are maintained at
[`lkarlslund/linux-x1e-t14s`](https://github.com/lkarlslund/linux-x1e-t14s):

- `t14s/jens-7.2rc6-pdc-v4`
- `t14s/mainline-edge`

The Jens PDC/SS3 branch carries Qualcomm's v4 series. It is experimental,
especially on firmware that leaves the PDC in secondary-controller mode.

## Jens 7.3 variant

`linux-x1e-jens73` pins Jens's `7.3.0-jg-1` tag
(`0151068ae447fd8f687b57837dd4be857cff7ca8`, branch `jg/ubuntu-qcom-x1e-7.3.y`).
It replaces and conflicts with `linux-x1e-t14s-edge` and the short-lived
`linux-x1e-jens73-sleep` test variant (matching headers likewise). Its loader
entry is `linux-x1e-jens73.conf`, titled **Arch Linux — Jens 7.3**.
Publishing this variant also retires old edge release assets.

The machine-local ASPM-v3 diagnostic loader entry pointed to the same old
kernel files; it has been preserved outside the entries directory as
`/boot/loader/linux-x1e-t14s-edge-aspm-v3.conf.disabled`.

Jens's 7.3.0 tree contains mainline v7.3, the clock `sync_state` series
(without it 7.3 hangs at boot with a black screen unless `clk_ignore_unused`
is passed), the camss PHY-API support matching the standalone CSI PHY nodes
(the camera fails to probe without it), Iris firmware loading without TZ/PAS,
and his EL2 remoteproc attach rework.

Pending upstream fixes carried here (linux-arm-msm, October 2026):

- `dispcc-x1e80100`: keep `disp_cc_pll0` out of the unused-clock sweep.
- `qcom_battmgr`: report `capacity` on X1E80100, report `Full` on external
  power, and announce AC/USB online changes for UPower.
- T14s PMIC thermal zones (Daniel Lezcano v3), which require
  `QCOM_SPMI_ADC5_GEN3`/`QCOM_SPMI_ADC_TM5_GEN3`.
- T14s: mark the EC reset GPIO as reserved.

X1E deep sleep at EL2 (s2idle + DRIPS), from justririll/x1e-sleep,
ramonjesusmiranda-debug/sp11-el2 and Anthony Ruhier, patches 0011-0020:

- system-sleep-only domain idle states; DRIPS (0x0200c354) and the CL5
  cluster state it is composed on are used only for system sleep (Jens drops
  CL5 from runtime idle because it resets X1 with DC ZVA), with the Oryon DC
  ZVA erratum workaround;
- arch timer: park stopped clockevents enabled+masked at 2^55-1 so the
  firmware's wake deadline is not in the past;
- icc-bwmon drops its DDR vote across suspend; syscon no longer holds the
  TCSR parent clock; crypto (QCE, no driver) disabled so the interconnects
  reach `sync_state`;
- `x1e_aoss_cxsd_only` (no AOSD, avoids hourly resets reported at EL2),
  loaded by the `x1e-el2-sleep` systemd-sleep hook, which also switches the
  sleep mode to s2idle; `x1e_el1tmr` is built but no longer used;
- raw PSCI s2idle failure logging.

Status (20 October 2026): DRIPS is entered and accepted, the CPU subsystem
collapses (~97 % `apss`), resume is clean, but CX collapse and DDR
self-refresh (`cxsd`/`ddr`) are not reached yet.

`_module_baseline=none` builds the T14s hardware configuration plus
`common/modules.config` only, without Ubuntu's generic ARM64 module set
(about 585 modules instead of 7,800; the 7.2.5 package keeps the Ubuntu
baseline). `modules.config` lists laptop peripherals explicitly (USB
Wi-Fi/Bluetooth/serial dongles, card readers, DisplayLink, input devices,
container networking, VFIO) and tracing (ftrace, kprobes, BPF events).

Only the four packaged T14s device trees are built, avoiding missing source
files for unrelated boards. Patch headers document provenance and rationale.

<details>
<summary>Superseded mainline 7.3-rc2 experiments</summary>

The edge variant pins mainline `28924df2a08f440c73991b83028032c901de2ae4`
with the direct PSR SDP flush fix and EL2 firmware compatibility changes.
PDC controller support,
pinctrl wake handling, SS3, the PDC register-span correction, and the T14s USB
PHY supply corrections are now upstream. The refresh preserves the existing
edge branch history and OLED EL2 boot layout. Boot and suspend/resume behavior
still require validation on the target machine.

Release `7.3.rc2-5` adds patches 1–6 of Manivannan Sadhasivam's July 8,
2026 PCI/ASPM v3 series (PCI core plus ath12k; unrelated ath10k/ath11k
conversions omitted). This uses `pci_force_enable_link_state()` rather
than the older v2 API behaviour in Jens's tree. Source:
https://patchew.org/linux/20260708-pci-aspm-fix-v3-0-6bd72451746e@kernel.org/
The local source pin is `be14bff37eea6224b5f44cdb532bf554a8428f93`, seeded
in makepkg under `refs/local-tests/aspm-v3`; it has not been published.
Release 4 boots to KDE and connects Wi-Fi, but Wi-Fi firmware restart
times out after deep suspend. Release 5 tests ASPM v3 as a candidate fix,
not a confirmed resolution. Firmware, DTB and PSR settings are unchanged.
Keep PDC as the default until deep suspend and Wi-Fi recovery are tested.

Release `7.3.rc2-4` restores the clock `sync_state` series from the working
Jens PDC kernel: `52c38d2eb73a`, `5d9de6624338`, `d4b1f8819533`,
`df47f8dd9b85`, and the include fix `5bcd1af67c4d`. Device-owned clocks
are swept after consumers have probed rather than during early global
unused-clock cleanup. Release 3 booted successfully with `clk_ignore_unused`
after hanging without it; release 4 tests the proper fix without that
workaround. PSR remains disabled while validating boot and graphics.
The source pin `405538e5606dfd722c9ac0030488d37eb5f0991e` is local until
testing/publishing, seeded in makepkg under `refs/local-tests/clock-sync`.
Keep PDC as the default; release 4 still requires a hardware boot test.

Release `7.3.rc2-3` was a diagnostic build: it reverts upstream commit
`5a8b2cc36e79` (X1E interconnect QoS configuration), with no other kernel
source or configuration changes relative to release 2. Release 2 hangs with
a black screen on this T14s, including with PSR and Plymouth disabled.
Other Snapdragon devices have reported resets resolved by this revert;
the different symptom here means it is not yet a confirmed fix.
See https://lkml.iu.edu/2609.0/15054.html and
https://lkml.iu.edu/2609.0/16456.html. Keep PDC as the fallback.
The diagnostic source commit is local until testing/publishing; local
makepkg source cache has it under `refs/local-tests/qos-revert`.

The initial `7.3.rc2-1` package failed to boot with ADSP stream `0x1000`
SMMU faults. Release `7.3.rc2-2` restores Jens's EL2 SHM bridge ownership
handling and 40-bit DMA window, removes the firmware-started DSP IOMMU
mappings, and honors `qcom,broken-reset` using the 7.3 remoteproc attach path.
Firmware reload/start is rejected for those DSPs and automatic recovery is
disabled because their reset path is unavailable in EL2. This avoids importing
the older attach-only ops with missing restart callbacks. A new boot test is
required; retain the known-working PDC kernel as the default fallback.

</details>

## Jens 7.2.5 PDC variant

`linux-x1e-jens725-pdc` pins Jens commit
`e57ec2987d4540fa89280d349f79c4d9bcd7cf27`. Its checksummed patch series lives
beside the recipe in this repository. It carries the rc6 PDC controller,
GPIO wake routing, SS3 idle state, register-span correction, and suspend
diagnostics. The USB PHY supply correction is already present upstream.
Keyboard wake and the rc6 boot options are retained. Jens's 7.2.5 EL2 overlay
change is retained: it no longer explicitly enables Iris or selects generic
video firmware. Hardware video acceleration therefore needs checking too.

It installs `linux-x1e-jens725-pdc.conf` with the title
**Arch Linux — Jens 7.2.5 + PDC/SS3 v4**.
The kernel, initramfs, modules and DTBs have separate paths. Installation does
not select it as the default. Boot and suspend/resume need hardware testing.

## Build

Verify all package definitions and collision boundaries:

```bash
./scripts/verify-packages
```

Build one kernel:

```bash
./scripts/build-kernels linux-x1e-jens725-pdc
```

Build all variants sequentially:

```bash
./scripts/build-kernels all
```

The local makepkg configuration controls `SRCDEST`, `PKGDEST`, and parallelism.
On this machine packages are written to `/home/lak/.cache/makepkg/pkg` and Git
sources are shared through `/home/lak/.cache/makepkg/src`.

Local builds keep each package's `src/` tree: `prepare()` resets it to the
pinned commit (`git reset --hard`, `git clean -fd`) before applying the
patches, while ignored build outputs stay, so a patch or pkgrel change only
rebuilds what it touches. Set `KERNEL_CLEANBUILD=1` for a from-scratch build;
CI always builds from scratch.

## Ubuntu ARM64 feature baseline

Both variants start from the checked-in `common/ubuntu-arm64.config`, a
saved Ubuntu ARM64 6.19.0-rc5 configuration, then retain the existing T14s
features, boot/runtime choices and Qualcomm fixes. This is a pinned snapshot,
not a claim to track the latest Ubuntu kernel automatically. See
`common/ubuntu-baseline.md` for provenance and the merge policy.

`scripts/prepare-kernel-config` resolves the existing hardware configuration on
the selected source revision, preserves its enabled features and scalar
settings, and overlays them onto Ubuntu's broad configuration. It also retains
disabled hardware choices for page size, address widths, preemption, graphics,
IOMMU mode, module signing and related boot policy. The positive settings in
`qcom_laptops.config` are retained; its optional reductions of other SoCs and
GPU drivers are omitted. `common/modules.config` adds explicit requirements,
including all NLS filename encodings and UTF-8.

After `make olddefconfig`, checks reject dropped T14s features and missing
required modules. Ubuntu module requests must resolve to modules or built-ins
when their symbols exist in the selected source. Removed/renamed symbols are
listed in `ubuntu-removed-symbols.txt` in the build tree. The small set of
incompatible existing symbols is documented in `common/ubuntu-module-exceptions`.
Unknown new omissions fail the build.

Use `iocharset=utf8` for CIFS mounts used from UTF-8 locales. Omitting it uses
the kernel's preserved default encoding, which may be ISO-8859-1. Building a
module does not start a service or configure a network interface.

To check a resolved configuration from its kernel source directory:

```bash
python /path/to/scripts/verify-kernel-config /path/to/common/modules.config .config \
  --ubuntu /path/to/common/ubuntu-arm64.config \
  --exceptions /path/to/common/ubuntu-module-exceptions
```

When changing any build input, update its checksum in `common/kernel-package.inc`
and bump `pkgrel` for affected variants that have already been built/released.

## GitHub builds and releases

GitHub Actions builds packages natively on ARM64 and publishes them to the
rolling `packages` release. Commits do not start builds. Run the workflow
manually and select one variant or all variants. The `build_headers` input is
off by default; enable it only when matching headers packages are needed. The
PDC diagnostic kernel is the default variant during current suspend testing.

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
sudo pacman -U /home/lak/.cache/makepkg/pkg/linux-x1e-jens725-pdc-*.pkg.tar.zst
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
known-good 7.2.5 PDC entry selected as the default until another variant has passed
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

## Keeping the running kernel's modules

Module directories carry the pkgrel (`7.3.0-7-x1e-jens73-ARCH`), so upgrading
a kernel package deletes the modules of the kernel that is still running, and
anything not yet loaded (USB storage, for example) fails until a reboot.
`packages/x1e-keep-running-modules` adds pacman hooks around every
`linux-x1e-*` transaction that hard-link the running tree aside and put it
back afterwards; its boot service removes the kept tree once another kernel
is running.
