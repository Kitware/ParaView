/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDWriter.h"

#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkKWMessageDialog.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkSMPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVReaderModule.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDWriter);
vtkCxxRevisionMacro(vtkPVDWriter, "1.9");

//----------------------------------------------------------------------------
vtkPVDWriter::vtkPVDWriter()
{
}

//----------------------------------------------------------------------------
vtkPVDWriter::~vtkPVDWriter()
{
}

//----------------------------------------------------------------------------
void vtkPVDWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkPVDWriter::CanWriteData(vtkDataSet* data, int, int)
{
  // We support all data types in both parallel and serial mode, and
  // with any number of parts.
  return data?1:0;
}

//----------------------------------------------------------------------------
void vtkPVDWriter::Write(const char* fileName, vtkPVSource* pvs,
                         int numProcs, int ghostLevel, int timeSeries)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkPVReaderModule* rm = vtkPVReaderModule::SafeDownCast(pvs);
  if(!rm)
    {
    timeSeries = 0;
    }

  // Create the writer.
  vtkClientServerID writerID = pm->NewStreamObject(timeSeries?
                                                   "vtkXMLPVAnimationWriter" :
                                                   "vtkXMLPVDWriter");
  pm->GetStream() << vtkClientServerStream::Invoke
                  << writerID << "SetNumberOfPieces" << numProcs
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << writerID << "SetFileName" << fileName
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << writerID << "SetGhostLevel" << ghostLevel
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << pm->GetProcessModuleID() << "GetPartitionId"
                  << vtkClientServerStream::End
                  << vtkClientServerStream::Invoke
                  << writerID << "SetPiece"
                  << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;

  // Tell each process's writer whether it should write the summary
  // file.
  if(numProcs > 1)
    {
    vtkClientServerID helperID = pm->NewStreamObject("vtkPVSummaryHelper");
    pm->GetStream() << vtkClientServerStream::Invoke
                    << helperID << "SetWriter" << writerID
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke
                    << pm->GetProcessModuleID() << "GetController"
                    << vtkClientServerStream::End
                    << vtkClientServerStream::Invoke
                    << helperID << "SetController"
                    << vtkClientServerStream::LastResult
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke
                    << helperID << "SynchronizeSummaryFiles"
                    << vtkClientServerStream::End;
    pm->DeleteStreamObject(helperID);
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER);

  if(timeSeries)
    {
    // Plug the inputs into the writer.
    int i;
    for(i=0; i < pvs->GetNumberOfParts(); ++i)
      {
      pm->GetStream() << vtkClientServerStream::Invoke
                      << writerID << "AddInput"
                      << pvs->GetPart(i)->GetID(0)
                      << pvs->GetName()
                      << vtkClientServerStream::End;
      }

    // Start the animation.
    pm->GetStream() << vtkClientServerStream::Invoke
                    << writerID << "Start"
                    << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);

    // Loop through all of the time steps.
    for(int t = 0; t < rm->GetNumberOfTimeSteps(); ++t)
      {
      // Update the data to the next time step.
      rm->SetRequestedTimeStep(t);

      // Write this time step.
      pm->GetStream() << vtkClientServerStream::Invoke
                      << writerID << "WriteTime" << t
                      << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::DATA_SERVER);
      }

    // Finish the animation.
    pm->GetStream() << vtkClientServerStream::Invoke
                    << writerID << "Finish"
                    << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    }
  else
    {
    // Plug the inputs into the writer.
    int i;
    for(i=0; i < pvs->GetNumberOfParts(); ++i)
      {
      pm->GetStream() << vtkClientServerStream::Invoke
                      << writerID << "AddInput"
                      << pvs->GetPart(i)->GetID(0)
                      << vtkClientServerStream::End;
      }

    // Just write the current data.
    pm->GetStream() << vtkClientServerStream::Invoke
                    << writerID << "Write"
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke
                    << writerID << "GetErrorCode"
                    << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    int retVal;
    if(pm->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &retVal) &&
       retVal == vtkErrorCode::OutOfDiskSpaceError)
      {
      vtkKWMessageDialog::PopupMessage(
        pvApp, pvApp->GetMainWindow(),
        "Write Error", "There is insufficient disk space to save this data. "
        "The file(s) already written will be deleted.");
      }
    }

  // Delete the writer.
  pm->DeleteStreamObject(writerID);
  pm->SendStream(vtkProcessModule::DATA_SERVER);
}
