# Shared kernel packaging implementation. Variant PKGBUILDs define the source,
# package identity, and boot-menu metadata before sourcing this file.

pkgdesc="Linux kernel for the Snapdragon X Elite ThinkPad T14s (${_variant_desc})"
url="${_kernel_repo}"
arch=(aarch64)
license=(GPL-2.0-only)
makedepends=(
  bc
  cpio
  dtc
  git
  kmod
  libelf
  pahole
  perl
  python
)
options=(
  !debug
  !strip
)

_srcname="${pkgbase}-${pkgver}"
source=(
  "${_srcname}::git+${_kernel_repo}.git#commit=${_commit}"
  'running.config'
  'linux.preset'
  '60-linux.hook'
  '90-linux.hook'
  'mkinitcpio.conf'
  'loader-entry.conf'
  "${pkgbase}.install"
)
sha256sums=(
  'SKIP'
  'f2e8613c4b593ecb6b9645d5a170dbb43af9075a21b16786ceef1ba788db4fb2'
  '01d1b6e61aee68ff3f27909cbed6b5458dade4e8807e229e04265f79777cc4ca'
  '452b8d4d71e1565ca91b1bebb280693549222ef51c47ba8964e411b2d461699c'
  '75f99f5239e03238f88d1a834c50043ec32b1dc568f2cc291b07d04718483919'
  'fb0959eb8859d36556fb9a1f29a2af2cf964961613c200a130322692940316aa'
  'SKIP'
  'SKIP'
)

export KBUILD_BUILD_HOST=archlinuxarm
export KBUILD_BUILD_USER=$pkgbase
export KBUILD_BUILD_TIMESTAMP="$(date -Ru${SOURCE_DATE_EPOCH:+d @$SOURCE_DATE_EPOCH})"
export LOCALVERSION=

prepare() {
  cd "${_srcname}"

  echo "-${pkgrel}" > localversion.10-pkgrel
  echo "${pkgbase#linux}" > localversion.20-pkgname

  awk '
    /^# CONFIG_[A-Za-z0-9_]+ is not set$/ { print; next }
    /^CONFIG_[A-Za-z0-9_]+=.*$/ { print; next }
  ' "${srcdir}/running.config" > .config

  if [[ -f arch/arm64/configs/qcom_laptops.config ]]; then
    awk '
      /^#/ { print; next }
      /^$/ { print; next }
      /^# CONFIG_[A-Za-z0-9_]+ is not set$/ { print; next }
      /^CONFIG_[A-Za-z0-9_]+=.*$/ { print; next }
    ' arch/arm64/configs/qcom_laptops.config > .qcom_laptops.config
    scripts/kconfig/merge_config.sh -m .config .qcom_laptops.config
  fi

  make olddefconfig
  make -s kernelrelease > version
  echo "Prepared ${pkgbase} $(<version) from ${_commit}"
}

build() {
  cd "${_srcname}"
  make -j"$(nproc)" Image Image.gz modules dtbs
}

_package() {
  pkgdesc="The ${pkgdesc} kernel and modules"
  depends=(
    coreutils
    initramfs
    kmod
  )
  optdepends=(
    'wireless-regdb: to set the correct wireless channels of your country'
  )
  provides=(WIREGUARD-MODULE)
  backup=("etc/mkinitcpio.d/${pkgbase}.preset")
  install="${pkgbase}.install"

  cd "${_srcname}"
  local kernver="$(<version)"
  local modulesdir="$pkgdir/usr/lib/modules/${kernver}"

  install -Dm644 arch/arm64/boot/Image.gz "$pkgdir/boot/vmlinuz-${pkgbase}"
  install -Dm644 arch/arm64/boot/Image "$pkgdir/boot/vmlinux-${pkgbase}"

  local dtb
  local -a t14s_dtbs=(arch/arm64/boot/dts/qcom/x1e78100-lenovo-thinkpad-t14s*.dtb)
  if [[ ! -e ${t14s_dtbs[0]} ]]; then
    error 'No ThinkPad T14s DTBs were built'
    return 1
  fi
  for dtb in "${t14s_dtbs[@]}"; do
    install -Dm644 "$dtb" "$pkgdir/boot/dtbs/${pkgbase}/$(basename "$dtb")"
  done

  local boot_subst="
    s|%PKGBASE%|${pkgbase}|g
    s|%BOOT_TITLE%|${_boot_title}|g
    s|%SORT_KEY%|${_sort_key}|g
    s|%KERNEL_OPTIONS%|${_kernel_options}|g
  "
  sed "${boot_subst}" "${srcdir}/loader-entry.conf" |
    install -Dm644 /dev/stdin "$pkgdir/boot/loader/entries/${pkgbase}.conf"

  echo "$pkgbase" | install -Dm644 /dev/stdin "$modulesdir/pkgbase"
  ZSTD_CLEVEL=19 make INSTALL_MOD_PATH="$pkgdir/usr" INSTALL_MOD_STRIP=1 \
    DEPMOD=/doesnt/exist modules_install
  rm -f "$modulesdir"/build "$modulesdir"/source

  local package_subst="
    s|%PKGBASE%|${pkgbase}|g
    s|%KERNVER%|${kernver}|g
  "
  sed "${package_subst}" "${srcdir}/linux.preset" |
    install -Dm644 /dev/stdin "$pkgdir/etc/mkinitcpio.d/${pkgbase}.preset"
  sed "${package_subst}" "${srcdir}/mkinitcpio.conf" |
    install -Dm644 /dev/stdin "$pkgdir/etc/mkinitcpio.conf.d/${pkgbase}.conf"
  sed "${package_subst}" "${srcdir}/60-linux.hook" |
    install -Dm644 /dev/stdin "$pkgdir/usr/share/libalpm/hooks/60-${pkgbase}.hook"
  sed "${package_subst}" "${srcdir}/90-linux.hook" |
    install -Dm644 /dev/stdin "$pkgdir/usr/share/libalpm/hooks/90-${pkgbase}.hook"

  touch "$modulesdir/vmlinuz"
}

_package-headers() {
  pkgdesc="Headers and scripts for building modules for the ${pkgdesc}"
  depends=(pahole)

  cd "${_srcname}"
  local builddir="$pkgdir/usr/lib/modules/$(<version)/build"

  install -Dt "$builddir" -m644 .config Makefile Module.symvers System.map \
    localversion.* version vmlinux
  install -Dt "$builddir/kernel" -m644 kernel/Makefile
  install -Dt "$builddir/arch/arm64" -m644 arch/arm64/Makefile
  cp -t "$builddir" -a scripts

  cp -t "$builddir" -a include
  cp -t "$builddir/arch/arm64" -a arch/arm64/include
  install -Dt "$builddir/arch/arm64/kernel" -m644 arch/arm64/kernel/asm-offsets.s
  mkdir -p "$builddir/arch/arm"
  cp -t "$builddir/arch/arm" -a arch/arm/include

  install -Dt "$builddir/drivers/md" -m644 drivers/md/*.h
  install -Dt "$builddir/net/mac80211" -m644 net/mac80211/*.h
  install -Dt "$builddir/drivers/media/i2c" -m644 drivers/media/i2c/msp3400-driver.h
  install -Dt "$builddir/drivers/media/usb/dvb-usb" -m644 drivers/media/usb/dvb-usb/*.h
  install -Dt "$builddir/drivers/media/dvb-frontends" -m644 drivers/media/dvb-frontends/*.h
  install -Dt "$builddir/drivers/media/tuners" -m644 drivers/media/tuners/*.h
  install -Dt "$builddir/drivers/iio/common/hid-sensors" -m644 drivers/iio/common/hid-sensors/*.h

  find . -name 'Kconfig*' -exec install -Dm644 {} "$builddir/{}" \;

  local archdir
  for archdir in "$builddir"/arch/*/; do
    [[ $archdir = */arm64/ || $archdir = */arm/ ]] && continue
    rm -r "$archdir"
  done
  rm -r "$builddir/Documentation"
  find -L "$builddir" -type l -delete
  find "$builddir" -type f -name '*.o' -delete

  local file
  while read -rd '' file; do
    case "$(file -Sib "$file")" in
      application/x-sharedlib\;*)
        strip -v $STRIP_SHARED "$file" ;;
      application/x-archive\;*)
        strip -v $STRIP_STATIC "$file" ;;
      application/x-executable\;*)
        strip -v $STRIP_BINARIES "$file" ;;
      application/x-pie-executable\;*)
        strip -v $STRIP_SHARED "$file" ;;
    esac
  done < <(find "$builddir" -type f -perm -u+x ! -name vmlinux -print0)

  mkdir -p "$pkgdir/usr/src"
  ln -sr "$builddir" "$pkgdir/usr/src/$pkgbase"
}

pkgname=(
  "$pkgbase"
  "$pkgbase-headers"
)
for _p in "${pkgname[@]}"; do
  eval "package_$_p() { _package${_p#$pkgbase}; }"
done
