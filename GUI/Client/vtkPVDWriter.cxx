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
vtkCxxRevisionMacro(vtkPVDWriter, "1.11");

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
int vtkPVDWriter::CanWriteData(vtkDataObject* data, int, int)
{
  // We support all dataset types in both parallel and serial mode, and
  // with any number of parts.
  if (!data || !data->IsA("vtkDataSet"))
    {
    return 0;
    }
  return 1;
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
  vtkClientServerStream stream;
  vtkClientServerID writerID = pm->NewStreamObject(
    timeSeries ?  "vtkXMLPVAnimationWriter" : "vtkXMLPVDWriter", stream);
  stream << vtkClientServerStream::Invoke
         << writerID << "SetNumberOfPieces" << numProcs
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << writerID << "SetFileName" << fileName
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << writerID << "SetGhostLevel" << ghostLevel
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetPartitionId"
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke
         << writerID << "SetPiece" << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;

  // Tell each process's writer whether it should write the summary
  // file.
  if(numProcs > 1)
    {
    vtkClientServerID helperID = 
      pm->NewStreamObject("vtkPVSummaryHelper", stream);
    stream << vtkClientServerStream::Invoke
           << helperID << "SetWriter" << writerID
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "GetController"
           << vtkClientServerStream::End
           << vtkClientServerStream::Invoke
           << helperID << "SetController" << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << helperID << "SynchronizeSummaryFiles"
           << vtkClientServerStream::End;
    pm->DeleteStreamObject(helperID, stream);
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

  if(timeSeries)
    {
    // Plug the inputs into the writer.
    int i;
    for(i=0; i < pvs->GetNumberOfParts(); ++i)
      {
      stream << vtkClientServerStream::Invoke
             << writerID << "AddInput" << pvs->GetPart(i)->GetID(0) << pvs->GetName()
             << vtkClientServerStream::End;
      }
    
    // Start the animation.
    stream << vtkClientServerStream::Invoke
           << writerID << "Start"
           << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

    // Loop through all of the time steps.
    for(int t = 0; t < rm->GetNumberOfTimeSteps(); ++t)
      {
      // Update the data to the next time step.
      rm->SetRequestedTimeStep(t);

      // Write this time step.
      stream << vtkClientServerStream::Invoke
             << writerID << "WriteTime" << t
             << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
      }

    // Finish the animation.
    stream << vtkClientServerStream::Invoke
           << writerID << "Finish"
           << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
    }
  else
    {
    // Plug the inputs into the writer.
    int i;
    for(i=0; i < pvs->GetNumberOfParts(); ++i)
      {
      stream << vtkClientServerStream::Invoke
             << writerID << "AddInput" << pvs->GetPart(i)->GetID(0)
             << vtkClientServerStream::End;
      }

    // Just write the current data.
    stream << vtkClientServerStream::Invoke
           << writerID << "Write"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << writerID << "GetErrorCode"
           << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
    int retVal;
    if(pm->GetLastResult(
         vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &retVal) &&
       retVal == vtkErrorCode::OutOfDiskSpaceError)
      {
      vtkKWMessageDialog::PopupMessage(
        pvApp, pvApp->GetMainWindow(),
        "Write Error", "There is insufficient disk space to save this data. "
        "The file(s) already written will be deleted.");
      }
    }

  // Delete the writer.
  pm->DeleteStreamObject(writerID, stream);
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
}
