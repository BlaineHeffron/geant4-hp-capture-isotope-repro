#!/bin/sh
# Build hpiso and print the target-isotope evidence for natural Pb, Cd and Gd.
# Requires a sourced Geant4 environment (geant4.sh), which puts geant4-config
# on PATH and sets G4NEUTRONHPDATA.
# Usage: ./run.sh [neutrons_per_run]   (default 100000)
set -eu
N=${1:-100000}
cd "$(dirname "$0")"
V=$(geant4-config --version)
B=build-$V
R=results/$V
cmake -S . -B "$B" -DGeant4_DIR="$(geant4-config --prefix)/lib/cmake/Geant4" >/dev/null
cmake --build "$B" >/dev/null
mkdir -p "$R"
seed=1
for mat in G4_Pb G4_Cd G4_Gd; do
  for mode in hp probe radcap radcapxs builder; do
    "$B/hpiso" "$mat" "$mode" "$N" "$seed" > "$R/$mat.$mode.log" 2>&1
    seed=$((seed + 1))
  done
done
python3 summary.py "$R"
