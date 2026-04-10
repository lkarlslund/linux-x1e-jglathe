# Maintainer: local custom build

pkgbase=linux-x1e-jglathe
pkgver=6.19.6
pkgrel=1
pkgdesc='Linux kernel for Snapdragon X Elite laptops (jglathe branch)'
url='https://github.com/jglathe/linux_ms_dev_kit'
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
_tag='jg/ubuntu-qcom-x1e-6.19.6-jg-6'
_srcname='linux_ms_dev_kit_6_19_6_jg_6'
source=(
  "${_srcname}::git+https://github.com/jglathe/linux_ms_dev_kit.git#tag=${_tag}"
  'running.config'
  'linux.preset'
  '60-linux.hook'
  '90-linux.hook'
  'mkinitcpio.conf'
  'linux-x1e-jglathe.install'
)
sha256sums=(
  'SKIP'
  '6262ad71700a31677353590576830eafc4b3bc020f679e14f26f783ab2641779'
  '01d1b6e61aee68ff3f27909cbed6b5458dade4e8807e229e04265f79777cc4ca'
  '452b8d4d71e1565ca91b1bebb280693549222ef51c47ba8964e411b2d461699c'
  '75f99f5239e03238f88d1a834c50043ec32b1dc568f2cc291b07d04718483919'
  'd417d09742b7a1f59f3bc2135d23a91fcf006629ba14336f812f89bc73d34e8e'
  '1a5c8d4a83c4cef7aa8dff107cdc2e7286cd1ecc9dfbc414e7d746623de734d1'
)

export KBUILD_BUILD_HOST=archlinuxarm
export KBUILD_BUILD_USER=$pkgbase
export KBUILD_BUILD_TIMESTAMP="$(date -Ru${SOURCE_DATE_EPOCH:+d @$SOURCE_DATE_EPOCH})"

prepare() {
  cd "${_srcname}"

  echo "Setting version..."
  echo "-${pkgrel}" > localversion.10-pkgrel
  echo "${pkgbase#linux}" > localversion.20-pkgname

  echo "Merging running config with qcom_laptops.config..."
  awk '
    /^# CONFIG_[A-Za-z0-9_]+ is not set$/ { print; next }
    /^CONFIG_[A-Za-z0-9_]+=.*$/ { print; next }
  ' "${srcdir}/running.config" > .config
  awk '
    /^#/ { print; next }
    /^$/ { print; next }
    /^# CONFIG_[A-Za-z0-9_]+ is not set$/ { print; next }
    /^CONFIG_[A-Za-z0-9_]+=.*$/ { print; next }
  ' arch/arm64/configs/qcom_laptops.config > .qcom_laptops.config
  scripts/kconfig/merge_config.sh -m .config .qcom_laptops.config
  make olddefconfig

  make -s kernelrelease > version
  echo "Prepared $pkgbase version $(<version)"
}

build() {
  cd "${_srcname}"
  make -j"$(nproc)" Image Image.gz modules dtbs
}

_package() {
  pkgdesc="The $pkgdesc kernel and modules"
  depends=(
    coreutils
    initramfs
    kmod
  )
  optdepends=(
    'wireless-regdb: to set the correct wireless channels of your country'
  )
  provides=(
    WIREGUARD-MODULE
  )
  backup=("etc/mkinitcpio.d/${pkgbase}.preset")
  install=${pkgbase}.install

  cd "${_srcname}"
  local kernver="$(<version)"
  local modulesdir="$pkgdir/usr/lib/modules/${kernver}"

  echo "Installing boot image..."
  install -Dm644 arch/arm64/boot/Image.gz "$pkgdir/boot/vmlinuz-${pkgbase}"
  install -Dm644 arch/arm64/boot/Image "$pkgdir/boot/vmlinux-${pkgbase}"

  echo "Installing DTBs..."
  shopt -s nullglob
  local dtb
  for dtb in arch/arm64/boot/dts/qcom/x1e*.dtb; do
    install -Dm644 "$dtb" "$pkgdir/boot/$(basename "$dtb")"
  done
  shopt -u nullglob

  # Used by mkinitcpio to name the kernel
  echo "$pkgbase" | install -Dm644 /dev/stdin "$modulesdir/pkgbase"

  echo "Installing modules..."
  ZSTD_CLEVEL=19 make INSTALL_MOD_PATH="$pkgdir/usr" INSTALL_MOD_STRIP=1 \
    DEPMOD=/doesnt/exist modules_install

  # remove build/source links
  rm -f "$modulesdir"/build "$modulesdir"/source

  local _subst="
    s|%PKGBASE%|${pkgbase}|g
    s|%KERNVER%|${kernver}|g
  "

  sed "${_subst}" "${srcdir}/linux.preset" |
    install -Dm644 /dev/stdin "$pkgdir/etc/mkinitcpio.d/${pkgbase}.preset"

  sed "${_subst}" "${srcdir}/mkinitcpio.conf" |
    install -Dm644 /dev/stdin "$pkgdir/etc/mkinitcpio.conf.d/${pkgbase}.conf"

  sed "${_subst}" "${srcdir}/60-linux.hook" |
    install -Dm644 /dev/stdin "$pkgdir/usr/share/libalpm/hooks/60-${pkgbase}.hook"

  sed "${_subst}" "${srcdir}/90-linux.hook" |
    install -Dm644 /dev/stdin "$pkgdir/usr/share/libalpm/hooks/90-${pkgbase}.hook"

  # trigger systemd kernel-install hooks where present
  touch "$modulesdir/vmlinuz"
}

_package-headers() {
  pkgdesc="Headers and scripts for building modules for the $pkgdesc kernel"
  depends=(pahole)

  cd "${_srcname}"
  local builddir="$pkgdir/usr/lib/modules/$(<version)/build"

  echo "Installing build files..."
  install -Dt "$builddir" -m644 .config Makefile Module.symvers System.map \
    localversion.* version vmlinux
  install -Dt "$builddir/kernel" -m644 kernel/Makefile
  install -Dt "$builddir/arch/arm64" -m644 arch/arm64/Makefile
  cp -t "$builddir" -a scripts

  echo "Installing headers..."
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

  echo "Installing KConfig files..."
  find . -name 'Kconfig*' -exec install -Dm644 {} "$builddir/{}" \;

  echo "Removing unneeded architectures..."
  local archdir
  for archdir in "$builddir"/arch/*/; do
    [[ $archdir = */arm64/ || $archdir = */arm/ ]] && continue
    rm -r "$archdir"
  done

  echo "Removing documentation..."
  rm -r "$builddir/Documentation"

  echo "Removing broken symlinks..."
  find -L "$builddir" -type l -delete

  echo "Removing loose objects..."
  find "$builddir" -type f -name '*.o' -delete

  echo "Stripping build tools..."
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

  echo "Adding symlink..."
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
