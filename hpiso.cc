// hpiso: which target isotope does nCapture hand its model when the
// cross-section data set is G4NeutronHPCaptureData?
//
// Usage: hpiso <material> <mode> <nNeutrons> <seed> [energy_eV]
//   material  any NIST material, e.g. G4_Pb, G4_Cd, G4_Gd
//   mode      hp     : nCapture model is G4ParticleHPCapture (Geant4 default)
//             probe  : nCapture model is a trivial wrapper that records the
//                      isotope in the G4Nucleus it is handed, then delegates
//                      to G4ParticleHPCapture
//             radcap : nCapture model is G4NeutronRadCaptureHP (stock Geant4
//                      model that uses the isotope it is handed)
//             radcapxs: G4NeutronRadCaptureHP with the isotope-wise data set
//                      G4NeutronHPCaptureXS instead of G4ParticleHPCaptureData
//                      (workaround check)
//             builder: nCapture set up by G4NeutronPHPBuilder::Build, i.e.
//                      whatever the reference HP physics lists use in this
//                      Geant4 version
//
// Geometry: a 40 m cube of <material>, gun at the centre.  Physics: neutron
// nCapture only (no elastic), so every neutron captures at the gun energy and
// the expected isotope fractions are abundance x sigma(n,g)(E) per isotope.
// The nCapture process is set up exactly as G4NeutronPHPBuilder does it:
// G4NeutronCaptureProcess (which adds G4NeutronCaptureXS) plus
// G4ParticleHPCaptureData on top, plus G4ParticleHPCapture below 20 MeV.

#include "G4BaryonConstructor.hh"
#include "G4Box.hh"
#include "G4BosonConstructor.hh"
#include "G4Element.hh"
#include "G4HadronicInteraction.hh"
#include "G4IonConstructor.hh"
#include "G4Isotope.hh"
#include "G4LeptonConstructor.hh"
#include "G4LogicalVolume.hh"
#include "G4MesonConstructor.hh"
#include "G4NeutronCaptureProcess.hh"
#include "G4NeutronHPCaptureXS.hh"
#include "G4NeutronPHPBuilder.hh"
#include "G4NeutronRadCaptureHP.hh"
#include "G4Neutron.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleHPCapture.hh"
#include "G4ParticleHPCaptureData.hh"
#include "G4ParticleHPManager.hh"
#include "G4ProcessManager.hh"
#include "G4RunManagerFactory.hh"
#include "G4ShortLivedConstructor.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4UserSteppingAction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPhysicsList.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VProcess.hh"
#include "G4Version.hh"
#include "Randomize.hh"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <sstream>
#include <string>

namespace {

G4String gMaterial;
G4String gMode;
G4double gEnergy = 0.0253 * eV;

// Filled by the probe model for the capture currently being processed.
G4int gHandedA = -1;
G4int gAfterA = -1;

std::map<G4int, long> gHanded;    // probe: isotope in the G4Nucleus handed to the model
std::map<G4int, long> gAfter;     // probe: isotope in the G4Nucleus after G4ParticleHPCapture
std::map<G4int, long> gResidual;  // both modes: (A of residual nucleus) - 1
long gCaptures = 0;

// Trivial custom model: record the target isotope it is handed, then
// delegate to the stock HP capture model so the final state is unchanged.
class IsotopeProbe : public G4HadronicInteraction
{
  public:
    explicit IsotopeProbe(G4HadronicInteraction* inner)
      : G4HadronicInteraction("IsotopeProbe"), fInner(inner)
    {
      SetMinEnergy(0.);
      SetMaxEnergy(20. * MeV);
    }
    G4HadFinalState* ApplyYourself(const G4HadProjectile& p, G4Nucleus& nuc) override
    {
      gHandedA = nuc.GetA_asInt();
      G4HadFinalState* fs = fInner->ApplyYourself(p, nuc);
      gAfterA = nuc.GetA_asInt();
      return fs;
    }
    void BuildPhysicsTable(const G4ParticleDefinition& pd) override
    {
      fInner->BuildPhysicsTable(pd);
    }
    void InitialiseModel() override { fInner->InitialiseModel(); }

  private:
    G4HadronicInteraction* fInner;
};

class PhysicsList : public G4VUserPhysicsList
{
  public:
    void ConstructParticle() override
    {
      G4BosonConstructor().ConstructParticle();
      G4LeptonConstructor().ConstructParticle();
      G4MesonConstructor().ConstructParticle();
      G4BaryonConstructor().ConstructParticle();
      G4IonConstructor().ConstructParticle();
      G4ShortLivedConstructor().ConstructParticle();
    }
    void ConstructProcess() override
    {
      AddTransportation();
      auto* cap = new G4NeutronCaptureProcess();      // adds G4NeutronCaptureXS
      G4Neutron::Neutron()->GetProcessManager()->AddDiscreteProcess(cap);
      if (gMode == "builder") {
        G4NeutronPHPBuilder().Build(cap);
        return;
      }
      if (gMode == "radcap" || gMode == "radcapxs") {
        auto* rc = new G4NeutronRadCaptureHP;
        rc->SetMinEnergy(0.);
        rc->SetMaxEnergy(20. * MeV);
        if (gMode == "radcap")
          cap->AddDataSet(new G4ParticleHPCaptureData);
        else
          cap->AddDataSet(new G4NeutronHPCaptureXS);
        cap->RegisterMe(rc);
        return;
      }
      cap->AddDataSet(new G4ParticleHPCaptureData);  // as G4NeutronPHPBuilder
      auto* hp = new G4ParticleHPCapture;
      hp->SetMinEnergy(0.);
      hp->SetMaxEnergy(20. * MeV);
      if (gMode == "probe")
        cap->RegisterMe(new IsotopeProbe(hp));
      else
        cap->RegisterMe(hp);
    }
};

class Detector : public G4VUserDetectorConstruction
{
  public:
    G4VPhysicalVolume* Construct() override
    {
      auto* mat = G4NistManager::Instance()->FindOrBuildMaterial(gMaterial);
      if (mat == nullptr) {
        std::fprintf(stderr, "unknown material %s\n", gMaterial.c_str());
        std::exit(2);
      }
      auto* box = new G4Box("World", 20 * m, 20 * m, 20 * m);
      auto* lv = new G4LogicalVolume(box, mat, "World");
      return new G4PVPlacement(nullptr, {}, lv, "World", nullptr, false, 0);
    }
};

class Gun : public G4VUserPrimaryGeneratorAction
{
  public:
    Gun() : fGun(1)
    {
      fGun.SetParticleDefinition(G4Neutron::Neutron());
      fGun.SetParticleEnergy(gEnergy);
      fGun.SetParticlePosition({0, 0, 0});
      fGun.SetParticleMomentumDirection({0, 0, 1});
    }
    void GeneratePrimaries(G4Event* ev) override { fGun.GeneratePrimaryVertex(ev); }

  private:
    G4ParticleGun fGun;
};

class Stepping : public G4UserSteppingAction
{
  public:
    void UserSteppingAction(const G4Step* step) override
    {
      const G4VProcess* proc = step->GetPostStepPoint()->GetProcessDefinedStep();
      if (proc == nullptr || proc->GetProcessName() != "nCapture") return;
      ++gCaptures;
      G4int Z = step->GetPreStepPoint()->GetMaterial()->GetElement(0)->GetZasInt();
      for (const G4Track* t : *step->GetSecondaryInCurrentStep()) {
        const G4ParticleDefinition* d = t->GetDefinition();
        if (d->GetAtomicNumber() == Z && d->GetAtomicMass() > 0) {
          ++gResidual[d->GetAtomicMass() - 1];
          break;
        }
      }
      if (gMode == "probe") {
        ++gHanded[gHandedA];
        ++gAfter[gAfterA];
      }
      // Only the captures matter: stop everything else.
      for (const G4Track* t : *step->GetSecondaryInCurrentStep())
        const_cast<G4Track*>(t)->SetTrackStatus(fStopAndKill);
    }
};

class Actions : public G4VUserActionInitialization
{
  public:
    void Build() const override
    {
      SetUserAction(new Gun);
      SetUserAction(new Stepping);
    }
};

// Pointwise (0 K) capture cross section of one isotope from G4NDL at energy e.
G4double NdlCaptureXS(G4int Z, G4int A, G4double e)
{
  const char* dir = std::getenv("G4NEUTRONHPDATA");
  if (dir == nullptr) return -1;
  std::string base = std::string(dir) + "/Capture/CrossSection/";
  std::string prefix = std::to_string(Z) + "_" + std::to_string(A) + "_";
  std::string file;
  for (const auto& f : std::filesystem::directory_iterator(base)) {
    std::string n = f.path().filename().string();
    if (n.rfind(prefix, 0) == 0) {
      file = base + n;
      break;
    }
  }
  if (file.empty()) return -1;
  if (file.size() > 2 && file.compare(file.size() - 2, 2, ".z") == 0)
    file.resize(file.size() - 2);
  std::istringstream in;
  G4ParticleHPManager::GetInstance()->GetDataStream(file, in);
  double h0, h1;
  long n;
  in >> h0 >> h1 >> n;
  double eV = e / CLHEP::eV, x0 = 0, s0 = 0;
  for (long i = 0; i < n; ++i) {
    double x, s;
    in >> x >> s;
    if (x >= eV) return i == 0 ? s : s0 + (s - s0) * (eV - x0) / (x - x0);
    x0 = x;
    s0 = s;
  }
  return -1;
}

void PrintRow(const char* label, const std::map<G4int, long>& h, G4int A, long tot)
{
  auto it = h.find(A);
  long c = it == h.end() ? 0 : it->second;
  std::printf("  %-26s %6ld  %7.4f\n", label, c, tot > 0 ? double(c) / tot : 0.);
}

}  // namespace

int main(int argc, char** argv)
{
  if (argc < 5) {
    std::fprintf(stderr, "usage: %s <material> <hp|probe|radcap|radcapxs|builder> <nNeutrons> <seed> [energy_eV]\n",
                 argv[0]);
    return 1;
  }
  gMaterial = argv[1];
  gMode = argv[2];
  long nEv = std::atol(argv[3]);
  long seed = std::atol(argv[4]);
  if (argc > 5) gEnergy = std::atof(argv[5]) * eV;
  if (gMode != "hp" && gMode != "probe" && gMode != "radcap" &&
      gMode != "radcapxs" && gMode != "builder") {
    std::fprintf(stderr, "mode must be hp, probe, radcap, radcapxs or builder\n");
    return 1;
  }
  G4Random::setTheSeed(seed);

  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
  rm->SetUserInitialization(new Detector);
  rm->SetUserInitialization(new PhysicsList);
  rm->SetUserInitialization(new Actions);
  rm->Initialize();
  rm->BeamOn((G4int)nEv);

  const G4Material* mat = G4NistManager::Instance()->FindOrBuildMaterial(gMaterial);
  const G4Element* el = mat->GetElement(0);
  const G4double* ab = el->GetRelativeAbundanceVector();
  G4double norm = 0;
  for (size_t j = 0; j < el->GetNumberOfIsotopes(); ++j)
    norm += ab[j] * NdlCaptureXS(el->GetZasInt(), el->GetIsotope(j)->GetN(), gEnergy);

  std::printf("\n=== %s  material=%s  mode=%s  E=%g eV  neutrons=%ld  captures=%ld  seed=%ld\n",
              G4Version.c_str(), gMaterial.c_str(), gMode.c_str(), gEnergy / eV, nEv,
              gCaptures, seed);
  std::printf("%-8s %9s %12s %9s | %-26s %6s  %7s\n", "isotope", "abundance", "sigma_ng[b]",
              "ab*sig", "observed", "count", "frac");
  for (size_t j = 0; j < el->GetNumberOfIsotopes(); ++j) {
    G4int A = el->GetIsotope(j)->GetN();
    G4double sig = NdlCaptureXS(el->GetZasInt(), A, gEnergy);
    std::printf("%-8s %9.5f %12.5g %9.5f |\n", el->GetIsotope(j)->GetName().c_str(), ab[j], sig,
                ab[j] * sig / norm);
    if (gMode == "probe") {
      PrintRow("handed to model", gHanded, A, gCaptures);
      PrintRow("after G4ParticleHPCapture", gAfter, A, gCaptures);
    }
    PrintRow("residual nucleus A-1", gResidual, A, gCaptures);
  }
  delete rm;
  return 0;
}
