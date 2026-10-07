# Ubuntu ARM64 baseline provenance

- Imported from `/home/lak/ubuntu-config`, a saved local Ubuntu kernel config.
- Header: `Linux/arm64 6.19.0-rc5 Kernel Configuration`.
- Compiler header: `aarch64-linux-gnu-gcc (Ubuntu 15.2.0-4ubuntu4) 15.2.0`.
- SHA-256: `c24a2ef369c342a46ada88aa5baf3cbea4457413a1f9c22084008a5c3c690d86`.
- Original package/version beyond that header is not recorded. This snapshot
  is used for feature coverage, not as a kernel source or an automatic update feed.
- 7,704 module requests. On PDC source `ca24f986dae338f166727f1965db49cbfb72a674`,
  7,623 are enabled, 76 symbols have been removed/renamed, and five have documented
  architecture/dependency conflicts. Counts can differ on the other variants.

## Preservation and verification

The existing variant configuration is resolved first. Every enabled feature
is retained, with built-ins kept built-in, except the derived
`ZRAM_BACKEND_FORCE_LZO` helper: it disappears when additional zram codecs are
available, while LZO support and the existing default compressor are retained.
I3C is promoted to built-in so adding it cannot demote existing I2C consumers.
The preparation script checks explicit hardware policy including disabled choices.
Ubuntu certificate paths and boot strings are overridden by the existing scalar
settings; it does not require Canonical's private build certificates.

Ubuntu's other module requests are checked after dependency resolution. A new
missing symbol that still exists in Kconfig is an error unless documented in
`ubuntu-module-exceptions`. Symbols removed from the source are reported for
review on each build; their old names cannot be enabled on that revision.

`modules.config` is an additional, strictly checked feature floor. It includes
UTF-8 and all NLS codepages, avoiding a kernel that has CIFS but cannot satisfy
`iocharset=utf8`. The baseline is shared by local makepkg and GitHub Actions.
