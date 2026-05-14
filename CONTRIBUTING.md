# Contributing

Thanks for considering a contribution!

## Quick checklist for a PR

1. The build passes locally: `pio run`
2. Code matches the surrounding style (header-only, single TU from `main.cpp`)
3. Pin and timing constants live in `src/config.h` — not magic numbers in the
   feature file
4. Any new behavior is reflected in the README

## Local development

```bash
git clone https://github.com/Capynaras-Club/BloomOnDevice
cd BloomOnDevice
pio run                    # compile
pio run -t upload          # flash a connected board
pio device monitor         # serial console
```

CI runs `pio run` for every push and PR; see
[`.github/workflows/build.yml`](.github/workflows/build.yml). The compiled
firmware is uploaded as an artifact on every run so you can download and
flash a build without a local toolchain.

## Reporting issues

Use the issue templates under
**Issues → New issue**. Please include:

- the board variant (DevKitM-1, custom, etc.)
- the arduino-esp32 / PlatformIO version
- a serial log at `CORE_DEBUG_LEVEL=4` if the issue is runtime behavior

## Releases

Tag a commit on `main` with `vX.Y.Z` (semver). The release workflow compiles
the firmware and attaches `firmware.bin`, `bootloader.bin`, and
`partitions.bin` to a GitHub Release automatically.

Pushes to `main` produce a rolling **nightly** pre-release with the same
artifacts, retagged on each push.

## License

By contributing, you agree your contributions are released under the same
[CC0 1.0 Universal](LICENSE) public-domain dedication that covers the rest
of this repository.
