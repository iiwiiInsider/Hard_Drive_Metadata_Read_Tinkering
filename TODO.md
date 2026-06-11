# TODO - EncryptDeat futuristic GUI + drive/integrity/security reporter

- [x] Implement futuristic blue/black custom UI (WM_PAINT) + dynamic controls

- [ ] Implement external drive enumeration (drive list + volume details)
- [ ] Implement best-effort encryption/protection detection (BitLocker via WMI if possible; otherwise mark unknown)
- [ ] Implement safe Integrity Checker report (last modified/metadata, health info) and “Run Integrity Check” UI
- [ ] Implement Settings system (load/save INI) + many “tinkering” options (safe mode by default)
- [ ] Export Report to file (JSON/text) from UI
- [ ] Build & test x64 Release; verify UI rendering and drive scanning
- [ ] Verify Unsafe Mode gate: OFF by default, locked unless elevated admin; enable requires 2-step confirmation

- [x] Hardening: disable destructive actions by default; require explicit multi-step confirmation for any risky placeholders

- [x] Add Unsafe Mode gate: admin required (elevated) + persisted toggle (SAFE by default) [UI toggle created in-window]



