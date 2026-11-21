# Reproduction steps

You need a Geant4 install with its data (G4NDL4.7.1 used here), CMake ≥ 3.16, a C++17 compiler and Python 3. Each `run.sh` takes about 2–3 minutes on one core. Seeds are fixed, so the same Geant4 build and data give exactly the output below.

```sh
git clone https://github.com/BlaineHeffron/geant4-hp-capture-isotope-repro.git
cd geant4-hp-capture-isotope-repro

# Geant4 11.3.2
. /path/to/geant4-11.3.2-install/bin/geant4.sh
./run.sh                 # builds into build-11.3.2/, writes results/11.3.2/*.log, prints the table

# Geant4 11.4.x (11.4.2 here)
. /path/to/geant4-11.4.2-install/bin/geant4.sh
./run.sh                 # build-11.4.2/, results/11.4.2/

# Re-print a table later:
python3 summary.py results/11.3.2

# A single run:  build-<ver>/hpiso <material> <mode> <neutrons> <seed> [energy_eV]
build-11.3.2/hpiso G4_Pb probe 100000 2
```

Setup:
- 25.3 meV neutrons start at the centre of a 40 m cube of G4_Pb, G4_Cd or G4_Gd.
- nCapture is the only neutron process, so every neutron captures at the gun energy.
- `expected` is abundance × σ(n,γ) from G4NDL pointwise data at 25.3 meV, normalised over the element.

Modes:

| mode | data set on nCapture | model on nCapture | column counts |
|---|---|---|---|
| `hp` | G4NeutronCaptureXS + G4ParticleHPCaptureData | G4ParticleHPCapture | residual nucleus A−1 |
| `probe` | same | trivial wrapper → G4ParticleHPCapture | isotope in the G4Nucleus handed to the wrapper |
| `radcap` | same | G4NeutronRadCaptureHP | residual A−1 |
| `radcapxs` | G4NeutronCaptureXS + G4NeutronHPCaptureXS | G4NeutronRadCaptureHP | residual A−1 (workaround) |
| `builder` | whatever `G4NeutronPHPBuilder::Build` sets up | (G4ParticleHPCapture in 11.3, G4NeutronRadCaptureHP in 11.4) | residual A−1 |

## Expected output

```
$Name: geant4-11-03-patch-02 [MT]$  G4_Pb  captures per run: 99999, 100000, 99997, 99997, 99998
  models: hp=NeutronHPCapture, probe=IsotopeProbe, radcap=nRadCaptureHP, radcapxs=nRadCaptureHP, builder=NeutronHPCapture
  isotope   expected       hp    probe   radcap radcapxs  builder
  Pb204       0.0642   0.0642   0.0143   0.0140   0.0646   0.0652
  Pb206       0.0417   0.0423   0.2414   0.2416   0.0428   0.0431
  Pb207       0.8933   0.8927   0.2193   0.2200   0.8918   0.8910
  Pb208       0.0008   0.0008   0.5250   0.5244   0.0008   0.0008
$Name: geant4-11-03-patch-02 [MT]$  G4_Cd  captures per run: 100000, 100000, 100000, 100000, 100000
  models: hp=NeutronHPCapture, probe=IsotopeProbe, radcap=nRadCaptureHP, radcapxs=nRadCaptureHP, builder=NeutronHPCapture
  isotope   expected       hp    probe   radcap radcapxs  builder
  Cd106       0.0000   0.0000   0.0121   0.0123   0.0000   0.0000
  Cd108       0.0000   0.0000   0.0086   0.0085   0.0000   0.0000
  Cd110       0.0006   0.0004   0.1269   0.1258   0.0005   0.0005
  Cd111       0.0004   0.0004   0.1278   0.1280   0.0004   0.0003
  Cd112       0.0002   0.0003   0.2425   0.2412   0.0002   0.0003
  Cd113       0.9988   0.9988   0.1229   0.1228   0.9989   0.9989
  Cd114       0.0000   0.0000   0.2846   0.2865   0.0000   0.0000
  Cd116       0.0000   0.0000   0.0747   0.0749   0.0000   0.0000
$Name: geant4-11-03-patch-02 [MT]$  G4_Gd  captures per run: 100000, 100000, 100000, 100000, 100000
  models: hp=NeutronHPCapture, probe=IsotopeProbe, radcap=nRadCaptureHP, radcapxs=nRadCaptureHP, builder=NeutronHPCapture
  isotope   expected       hp    probe   radcap radcapxs  builder
  Gd152       0.0000   0.0001   0.0021   0.0021   0.0000   0.0000
  Gd154       0.0000   0.0001   0.0223   0.0218   0.0001   0.0001
  Gd155       0.1850   0.1842   0.1481   0.1480   0.1847   0.1860
  Gd156       0.0000   0.0000   0.2048   0.2041   0.0000   0.0000
  Gd157       0.8149   0.8156   0.1560   0.1569   0.8152   0.8139
  Gd158       0.0000   0.0000   0.2469   0.2467   0.0000   0.0000
  Gd160       0.0000   0.0000   0.2198   0.2204   0.0000   0.0000
$Name: geant4-11-04-patch-02 [MT]$  G4_Pb  captures per run: 99999, 100000, 99998, 99997, 99996
  models: hp=NeutronHPCapture, probe=IsotopeProbe, radcap=nRadCaptureHP, radcapxs=nRadCaptureHP, builder=nRadCaptureHP
  isotope   expected       hp    probe   radcap radcapxs  builder
  Pb204       0.0642   0.0642   0.0143   0.0138   0.0625   0.0142
  Pb206       0.0417   0.0423   0.2414   0.2409   0.0426   0.2390
  Pb207       0.8933   0.8927   0.2193   0.2191   0.8940   0.2222
  Pb208       0.0008   0.0008   0.5250   0.5262   0.0009   0.5246
$Name: geant4-11-04-patch-02 [MT]$  G4_Cd  captures per run: 100000, 100000, 100000, 100000, 100000
  models: hp=NeutronHPCapture, probe=IsotopeProbe, radcap=nRadCaptureHP, radcapxs=nRadCaptureHP, builder=nRadCaptureHP
  isotope   expected       hp    probe   radcap radcapxs  builder
  Cd106       0.0000   0.0000   0.0121   0.0123   0.0000   0.0126
  Cd108       0.0000   0.0000   0.0086   0.0086   0.0000   0.0085
  Cd110       0.0006   0.0004   0.1269   0.1251   0.0005   0.1226
  Cd111       0.0004   0.0004   0.1278   0.1279   0.0004   0.1296
  Cd112       0.0002   0.0003   0.2425   0.2413   0.0003   0.2393
  Cd113       0.9988   0.9988   0.1229   0.1221   0.9988   0.1241
  Cd114       0.0000   0.0000   0.2846   0.2873   0.0001   0.2874
  Cd116       0.0000   0.0000   0.0747   0.0755   0.0000   0.0759
$Name: geant4-11-04-patch-02 [MT]$  G4_Gd  captures per run: 100000, 100000, 100000, 100000, 100000
  models: hp=NeutronHPCapture, probe=IsotopeProbe, radcap=nRadCaptureHP, radcapxs=nRadCaptureHP, builder=nRadCaptureHP
  isotope   expected       hp    probe   radcap radcapxs  builder
  Gd152       0.0000   0.0001   0.0021   0.0021   0.0000   0.0021
  Gd154       0.0000   0.0001   0.0223   0.0218   0.0001   0.0217
  Gd155       0.1850   0.1842   0.1481   0.1480   0.1847   0.1476
  Gd156       0.0000   0.0000   0.2048   0.2041   0.0000   0.2051
  Gd157       0.8149   0.8156   0.1560   0.1569   0.8152   0.1572
  Gd158       0.0000   0.0000   0.2469   0.2467   0.0000   0.2489
  Gd160       0.0000   0.0000   0.2198   0.2204   0.0000   0.2174
```

## What this shows

- **probe:** with G4ParticleHPCaptureData, the isotope handed to the model on nCapture follows natural abundance (Pb208 0.525, Cd113 0.123, Gd157 0.156). It should follow abundance × σ (0.0008, 0.999, 0.815).
- **hp:** G4ParticleHPCapture is still right, because it re-samples the isotope internally and overwrites the target (`G4NeutronHPCapture.cc:100-117`).
- **radcap:** a model that uses the isotope it is handed gets the abundance draw.
- **builder:** in 11.4 the reference-list builder registers such a model, so FTFP_BERT_HP, Shielding and the INCLXX HP lists are wrong in 11.4. QGSP_BIC_HP is wrong too, through its own setup (`G4HadronPhysicsQGSP_BIC_HP.cc:130-136`).
- **radcapxs:** swapping in G4NeutronHPCaptureXS, which is isotope-wise, restores the correct fractions.

Cause: `G4NeutronHPCaptureData::GetIsoCrossSection` (11.3.2 `G4NeutronHPCaptureData.cc:90-106`) ignores Z, A and the isotope and returns the element cross section. `G4CrossSectionDataStore::SampleZandA` (`G4CrossSectionDataStore.cc:237-266`) then weights isotopes by abundance × element σ. Reported as Geant4 Bugzilla #2782.
