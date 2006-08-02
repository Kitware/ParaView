/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVEWriter.h"

#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkKWMessageDialog.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkSMPart.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVReaderModule.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
// ATTRIBUTE EDITOR
#include "vtkPVAttributeEditor.h"
//#include "vtkPVLabeledToggle.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVFileEntry.h"
//#include "vtkVector.txx"
//#include "vtkVectorIterator.txx"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVEWriter);
vtkCxxRevisionMacro(vtkPVEWriter, "1.5");

//----------------------------------------------------------------------------
vtkPVEWriter::vtkPVEWriter()
{
}

//----------------------------------------------------------------------------
vtkPVEWriter::~vtkPVEWriter()
{
}

//----------------------------------------------------------------------------
void vtkPVEWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkPVEWriter::CanWriteData(vtkDataObject* data, int parallel, int vtkNotUsed(numParts))
{
  if (data == NULL)
    {
    return 0;
    }
  if (parallel && !this->Parallel)
    {
    return 0;
    }
  // don't check numParts because for attribute editor, it will have mutiple parts due to its multiple inputs
  return (data->IsA(this->InputClassName));
}

//----------------------------------------------------------------------------
int vtkPVEWriter::WriteOneFile(const char* fileName, vtkPVSource* pvs,
                              int numProcs, int ghostLevel)
{

  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerID dataID = pvs->GetPart()->GetID(0);
  int success = 1;

  vtkPVAttributeEditor *editor = vtkPVAttributeEditor::SafeDownCast(pvs);
  if(editor)
    {
    // This tells the filter to read from the "Source" input so that all of the data is available to the writer
    editor->SetPassSourceInput(1);
    // This makes sure the filter does not edit the data
    editor->SetForceNoEdit(1);
    editor->AcceptCallback();
    editor->SetForceNoEdit(0);
    }

  // Create the writer and configure it.
  vtkClientServerStream stream;
  vtkClientServerID writerID = 
    pm->NewStreamObject(this->WriterClassName, stream);
  stream << vtkClientServerStream::Invoke
         << writerID << "SetFileName" << fileName
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << writerID << "SetInput" << dataID
         << vtkClientServerStream::End;
  if (this->DataModeMethod)
    {
    stream << vtkClientServerStream::Invoke
           << writerID << this->DataModeMethod
           << vtkClientServerStream::End;
    }

  if(this->Parallel)
    {
    stream << vtkClientServerStream::Invoke
           << writerID << "SetGhostLevel" << ghostLevel
           << vtkClientServerStream::End;
    if (strstr(this->WriterClassName, "XMLP"))
      {
      stream << vtkClientServerStream::Invoke
             << writerID << "SetNumberOfPieces" << numProcs
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << pm->GetProcessModuleID() << "GetPartitionId"
             << vtkClientServerStream::End
             << vtkClientServerStream::Invoke
             << writerID << "SetStartPiece" << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << pm->GetProcessModuleID() << "GetPartitionId"
             << vtkClientServerStream::End
             << vtkClientServerStream::Invoke
             << writerID << "SetEndPiece" << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
  
      // Tell each process's writer whether it should write the summary
      // file.  This assumes that the writer is a vtkXMLWriter.  When we
      // add more writers, we will need a separate writer module.
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
    }

  // ATTRIBUTE EDITOR
  // For now, only exodus data can be written out
  // To modify a writer to make use of the Attribute Editor filter (or atleast this is what I did for the vtkExodusIIWriter):
  //   1) Use an EditorFlag var in your writer to differentiate between a "normal" save and one where you are writing out a single attribute array
  //   2) Use an EditedVariableName var in your writer to find what array to write
  //   2) It needs to use the vtkModelMetaData class to pass meta data about the timestep, the order of variable names, etc.
  //   3) modify Writers.xml to allow the different file extensions you might want to write to using the writer
  //   4) make sure the writer has access to the to a mapping between the 'actual' point/cell ids in the file and the ones assigned just for the ParaView session (if there is a difference)
  if(editor && strcmp(this->WriterClassName,"vtkExodusIIWriter")==0)
    {
    vtkPVArrayMenu *array = vtkPVArrayMenu::SafeDownCast(editor->GetPVWidget("Scalars"));
    // Setting this flag tells the writer that:
    //   1) we are writing back to the original file
    //   2) we are replacing a single attribute array in the data file
    stream << vtkClientServerStream::Invoke
                    << writerID << "SetEditorFlag" << 1
                    << vtkClientServerStream::End;
    int originalFileFlag = 0;
    if(strcmp(fileName,this->GetPVApplication()->GetMainWindow()->GetCurrentPVReaderModule()->GetFileEntry()->GetValue())==0)
      originalFileFlag = 1;
    stream << vtkClientServerStream::Invoke
                    << writerID << "SetWritingToOriginalFile" << originalFileFlag
                    << vtkClientServerStream::End;
    // Pass the variable name that was edited on to the writer:
    stream << vtkClientServerStream::Invoke
                    << writerID << "SetEditedVariableName" << array->GetValue()
                    << vtkClientServerStream::End;
    }

  // Write the data.
  stream << vtkClientServerStream::Invoke
         << writerID << "Write"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << writerID << "GetErrorCode"
         << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
    vtkProcessModule::DATA_SERVER, stream);
  int retVal;
  if(pm->GetLastResult(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
      vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &retVal) &&
    retVal == vtkErrorCode::OutOfDiskSpaceError)
    {
    vtkKWMessageDialog::PopupMessage(
      pvApp, pvApp->GetMainWindow(),
      "Write Error", "There is insufficient disk space to save this data. "
      "The file(s) already written will be deleted.");
    success = 0;
    }

  if(editor)
    {
    // This tells the filter to read from the "Filter" input once again
    editor->SetPassSourceInput(0);
    // This makes sure the filter does not edit the data
    editor->SetForceNoEdit(1);
    editor->AcceptCallback();
    editor->SetForceNoEdit(0);
    // This is a flag that keeps track whether or not the current timestep has been edited yet
    editor->SetEditedFlag(0);
    }

  // Cleanup.
  pm->DeleteStreamObject(writerID, stream);
  pm->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
    vtkProcessModule::DATA_SERVER, stream);
  return success;
}
