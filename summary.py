#!/usr/bin/env python3
"""Tabulate hpiso logs: fraction of captures per target isotope, by mode.

Usage: python3 summary.py results/<geant4-version>
Columns: expected = abundance x sigma(n,g) (G4NDL pointwise at the gun energy),
hp = G4ParticleHPCapture, probe = isotope handed to a custom model,
radcap = G4NeutronRadCaptureHP with G4ParticleHPCaptureData,
radcapxs = G4NeutronRadCaptureHP with G4NeutronHPCaptureXS (workaround), builder = G4NeutronPHPBuilder (reference lists).
hp, radcap, radcapxs and builder count the residual nucleus (A-1).
"""
import re
import sys
from pathlib import Path

MODES = ["hp", "probe", "radcap", "radcapxs", "builder"]


def parse(log):
    text = log.read_text()
    head = re.search(r"^=== (.*)$", text, re.M).group(1)
    model = re.search(r"Process: nCapture\s*\n\s*Model:\s*(\S+?):", text).group(1)
    rows, iso = {}, None
    for line in text[text.index("=== "):].splitlines()[2:]:
        m = re.match(r"(\w+)\s+([\d.]+)\s+\S+\s+([\d.]+) \|", line)
        if m:
            iso = m.group(1)
            rows[iso] = {"expected": float(m.group(3))}
            continue
        m = re.match(r"\s+(handed to model|residual nucleus A-1)\s+\d+\s+([\d.]+)", line)
        if m:
            rows[iso]["handed" if m.group(1).startswith("handed") else "residual"] = float(m.group(2))
    return head, model, rows


def main(d):
    d = Path(d)
    for mat in ["G4_Pb", "G4_Cd", "G4_Gd"]:
        data, models = {}, {}
        for mode in MODES:
            head, model, rows = parse(d / f"{mat}.{mode}.log")
            data[mode], models[mode] = rows, model
        print(f"\n{head.split('material=')[0].strip()}  {mat}  captures per run: "
              + ", ".join(re.search(r"captures=(\d+)", parse(d / f"{mat}.{m}.log")[0]).group(1) for m in MODES))
        print("  models: " + ", ".join(f"{m}={models[m]}" for m in MODES))
        print(f"  {'isotope':8} {'expected':>9} {'hp':>8} {'probe':>8} {'radcap':>8} {'radcapxs':>8} {'builder':>8}")
        for iso, r in data["hp"].items():
            print(f"  {iso:8} {r['expected']:9.4f} {r['residual']:8.4f} "
                  f"{data['probe'][iso]['handed']:8.4f} {data['radcap'][iso]['residual']:8.4f} "
                  f"{data['radcapxs'][iso]['residual']:8.4f} "
                  f"{data['builder'][iso]['residual']:8.4f}")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "results")
