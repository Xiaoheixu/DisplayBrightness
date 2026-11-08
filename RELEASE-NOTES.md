## DisplayBrightness 0.0.4

This prerelease adds external-monitor DDC/CI compatibility improvements, additional macOS profiles, and startup diagnostics. Compatible monitors use the native macOS brightness controls to adjust hardware brightness.

### Added

- DDC/CI reply-mode fallback for external-display interfaces that advertise framed replies. A failed framed attempt returns the remaining startup probes to ordinary I²C reads.
- Matching IOGraphics profiles for Ventura, Sonoma, and Sequoia, enabled with `-dbcompat`.
- Experimental endpoint recovery after eligible display-mode changes, with optional HDR recovery tracing, delay, and probe-window controls.
- `-dbbootlog` startup checkpoints and the manual `ExportBootLog.command` TXT exporter, including framebuffer topology and DDC probe results.

### Fixed

- Failed DDC requests now stop at a bounded retry limit. New brightness input can start another attempt.
- Displays waiting for the shared DDC bus yield their workers and retain the latest request. Pending probes can resume when a slot becomes available.
- Startup probes that have expired skip hardware I/O. A brightness write also rechecks the verified reply mode so it can be read back.
- DisplayBrightness preserves an existing native brightness provider and retires its own endpoint when the system takes over.

### Downloads and boot arguments

- **RELEASE ZIP** for general use. **DEBUG ZIP** includes additional startup logging and a matching `.dSYM`. Both include the TXT exporter. Use the SHA-256 file to verify your download.
- Install one `DisplayBrightness.kext` after Lilu in OpenCore. Requires x86_64, Lilu 1.7.3+, a matching IOGraphics profile, and a working DDC/CI path.
- Use `-dbcompat` for matching older macOS profiles, `-dbbootlog` for startup diagnostics, and `-dboff` to disable the plugin. `-dbhdr` opts into experimental mode-change recovery. Hardware control in HDR depends on the monitor accepting DDC/CI in that mode.

See the [README](https://github.com/Xiaoheixu/DisplayBrightness/blob/0.0.4/README.md) for the complete boot argument reference, installation steps, and what to include in a report.
