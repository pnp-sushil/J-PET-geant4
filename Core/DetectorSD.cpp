/**
 *  @copyright Copyright 2019 The J-PET Monte Carlo Authors. All rights reserved.
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may find a copy of the License in the LICENCE file.
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  @file DetectorSD.cpp
 */

#include "../Info/PrimaryParticleInformation.h"
#include <G4PrimaryParticle.hh>
#include <G4VProcess.hh>
#include "DetectorSD.h"
#include <algorithm>
#include <G4SystemOfUnits.hh>

DetectorSD::DetectorSD(G4String name, G4int scinSum, G4double timeMergeValue) :
  G4VSensitiveDetector(name), totScinNum(scinSum), timeIntervals(timeMergeValue)
{
  collectionName.insert("detectorCollection");
  previousHits.resize(totScinNum + 1);
}

DetectorSD::~DetectorSD() {}

void DetectorSD::Initialize(G4HCofThisEvent* HCE)
{
  static int HCID = -1;
  fDetectorCollection = new DetectorHitsCollection(SensitiveDetectorName, collectionName[0]);

  if (HCID < 0) {
    HCID = GetCollectionID(0);
  }
  HCE->AddHitsCollection(HCID, fDetectorCollection);

  std::fill(previousHits.begin(), previousHits.end(), HitParameters());
}

G4bool DetectorSD::ProcessHits(G4Step* aStep, G4TouchableHistory*)
{
  G4double edep = aStep->GetTotalEnergyDeposit();
  if (edep == 0.0) {
    if ( (aStep->GetPostStepPoint()->GetMomentum().mag2()-aStep->GetPreStepPoint()->GetMomentum().mag2()) > 0 ) {
      // particle quanta interact in detector but does not deposit energy
      // (vide Rayleigh scattering)
      if (aStep->GetTrack()->GetParentID() == 0) {
        PrimaryParticleInformation* info = dynamic_cast<PrimaryParticleInformation*> (aStep->GetTrack()->GetDynamicParticle()->GetPrimaryParticle()->GetUserInformation());
        if (info != nullptr) {
          info->SetGammaMultiplicity(0);
        }
      }
    }
    return false;
  }

  G4TouchableHistory* theTouchable = (G4TouchableHistory*)(aStep->GetPreStepPoint()->GetTouchable());
  G4VPhysicalVolume* physVol = theTouchable->GetVolume();
  G4int currentScinCopy = physVol->GetCopyNo();
  G4double currentTime = aStep->GetPreStepPoint()->GetGlobalTime();
  if (
    (previousHits[currentScinCopy].fID != -1)
    && (abs(previousHits[currentScinCopy].fTime - currentTime) < timeIntervals)
  ) {
    //! update track
    (*fDetectorCollection)[previousHits[currentScinCopy].fID]->AddEdep(edep);
    (*fDetectorCollection)[previousHits[currentScinCopy].fID]->AddInteraction();
    (*fDetectorCollection)[previousHits[currentScinCopy].fID]->AddTime(currentTime, edep);
    (*fDetectorCollection)[previousHits[currentScinCopy].fID]->AddPosition(aStep->GetPostStepPoint()->GetPosition(), edep);

  } else {
    //! new hit - interaction types compton, msc (multiple compton scatterings)
    DetectorHit* newHit = new DetectorHit();
    newHit->SetEdep(edep);
    newHit->SetTrackID(aStep->GetTrack()->GetTrackID());
    newHit->SetTrackPDG(aStep->GetTrack()->GetParticleDefinition()->GetPDGEncoding());
    newHit->SetProcessName(aStep->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName());
    newHit->SetInteractionNumber();
    newHit->SetPosition(aStep->GetPostStepPoint()->GetPosition(), edep);
    newHit->SetTime(currentTime, edep);
    newHit->SetScinID(physVol->GetCopyNo());
    newHit->SetPolarizationIn(aStep->GetPreStepPoint()->GetPolarization());
    newHit->SetMomentumIn(aStep->GetPreStepPoint()->GetMomentum());
    newHit->SetPolarizationOut(aStep->GetPostStepPoint()->GetPolarization());
    newHit->SetMomentumOut(aStep->GetPostStepPoint()->GetMomentum());

    //! only particles generated by user has PrimaryParticleInformation
    if (aStep->GetTrack()->GetParentID() == 0) {
      PrimaryParticleInformation* info =
        static_cast<PrimaryParticleInformation*> (
          aStep->GetTrack()->GetDynamicParticle()
          ->GetPrimaryParticle()->GetUserInformation()
        );

      if (info != 0) {
        newHit->SetGenGammaMultiplicity(info->GetGammaMultiplicity());
        newHit->SetGenGammaIndex(info->GetIndex());
        //! should be marked as scattering
        info->SetGammaMultiplicity(info->GetGammaMultiplicity() + 100);
      }
    }

    G4int id = fDetectorCollection->insert(newHit);
    previousHits[currentScinCopy].fID = id - 1;
    previousHits[currentScinCopy].fTime = currentTime;
  }
  return true;
}
