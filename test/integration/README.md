# Integration tests

End-to-end checks for backend emit and hardware-oriented paths (offline validation first).

## Scope

- Mock backends: format and gate-set checks without live hardware
- Hardware profiles: `tianyan`, `originquantum` (secrets via env; skipped in default CI)
- Driver / `-qpu-config` / `backend.ini` consistency with `test/qpu_configs/`

## Status

Scaffolding only. Add smoke tests as backend adapters stabilize.
