# geant4-hp-capture-isotope-repro

Minimal reproducer: with `G4NeutronHPCaptureData` on nCapture, the target isotope handed to the capture model is drawn by natural abundance, not abundance × σ(n,γ). `G4ParticleHPCapture` hides this by re-sampling internally. Models that use the isotope they are handed get the wrong target: `G4NeutronRadCaptureHP`, `G4NuDEXNeutronCaptureModel` and user models. In 11.4 that is also the HP reference-list builders.

Geant4 Bugzilla: #2782. Steps and expected output: [REPRO.md](REPRO.md).

Files: `hpiso.cc` (the whole program), `CMakeLists.txt`, `run.sh` (build + all runs), `summary.py` (table from the logs).
