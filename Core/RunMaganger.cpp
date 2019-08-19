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
 *  @file RunManager.cpp
 */

#include "../Actions/EventAction.h"
#include "RunManager.h"

void RunManager::DoEventLoop(G4int n_event, const char* macroFile, G4int n_select)
{
  InitializeEventLoop(n_event, macroFile, n_select);

  printf("\n\n");
  //! Event loop
  for (G4int i_event = 0; i_event < n_event; i_event++ ) {

    if (
      fEvtMessenger->PrintStatistics()
      && (i_event % int(pow(10, fEvtMessenger->GetPowerPrintStat())) == 0)
    ) {
      printf (" === Processed %i events \n", i_event);
    }

    if (fEvtMessenger->ShowProgress()) {
      printf (" === Progress %4.2f === \r", double(100 * i_event) / double(n_event));
    }

    if (fEvtMessenger->KillEventsEscapingWorld()) {
      bool isAborted = true;
      while (isAborted) {
        ProcessOneEvent(i_event);
         isAborted = currentEvent->IsAborted(); 
        if (isAborted) {
          //! clean event - it will not be stored
          delete currentEvent;
        }
      }


    } else if (fEvtMessenger->SaveOnlySelectedEvents()) {
      bool requirement = false;
      while (!requirement) {
        ProcessOneEvent(i_event);
        if (fEvtMessenger->Save2gEvents()) {
          requirement = EventAction::Is2gRegistered();
        } else if (fEvtMessenger->Save3gEvents()) {
          requirement = EventAction::Is3gRegistered();
        } else {
          G4Exception(
            "RunManager", "RM01", FatalException, " This can not happened "
          );
        }
        if (!requirement) {
          //! clean event - it will not be stored
          delete currentEvent;
        }
      }
    } else {
      ProcessOneEvent(i_event);
    }
    //! updating counters
    TerminateOneEvent();
    if (runAborted) break;
  }
  // For G4MTRunManager, TerminateEventLoop() is invoked after all threads are finished.
  if (runManagerType == sequentialRM) TerminateEventLoop();
}
