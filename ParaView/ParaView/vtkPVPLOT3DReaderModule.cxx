/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPLOT3DReaderModule.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVPLOT3DReaderModule.h"

#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkPLOT3DReader.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVData.h"
#include "vtkPVPart.h"
#include "vtkPVLabeledToggle.h"
#include "vtkPVRenderView.h"
#include "vtkPVSelectionList.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVWindow.h"
#include "vtkSource.h"
#include "vtkPVPassThrough.h"
#include "vtkStructuredGrid.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPLOT3DReaderModule);
vtkCxxRevisionMacro(vtkPVPLOT3DReaderModule, "1.9");

int vtkPVPLOT3DReaderModuleCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPLOT3DReaderModule::vtkPVPLOT3DReaderModule()
{
  this->CommandFunction = vtkPVPLOT3DReaderModuleCommand;
  this->PackFileEntry = 0;
}

//----------------------------------------------------------------------------
vtkPVPLOT3DReaderModule::~vtkPVPLOT3DReaderModule()
{
}

//----------------------------------------------------------------------------
void vtkPVPLOT3DReaderModule::Accept(int hideFlag, int hideSource)
{
  const char* tclName = this->GetVTKSourceTclName();
  char *outputTclName, *connectionTclName;
  vtkPVData *pvd = 0;
  vtkDataSet *d;
  int i;
  vtkPVWindow* window = this->GetPVWindow();
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPLOT3DReader* reader = vtkPLOT3DReader::SafeDownCast(
    this->GetVTKSource());

  this->UpdateVTKSourceParameters();
  if (!reader->CanReadFile(reader->GetFileName()))
    {
    vtkErrorMacro(<< "Can not read input file. Try changing parameters.");
    if (this->Initialized)
      {
      this->UnGrabFocus();
      this->SetAcceptButtonColorToWhite();
      }
#ifdef _WIN32
    this->Script("%s configure -cursor arrow", window->GetWidgetName());
#else
    this->Script("%s configure -cursor left_ptr", window->GetWidgetName());
#endif  
    return;
    }

  vtkPVWidget *pvw = 0;
  this->Widgets->InitTraversal();
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    pvw = this->Widgets->GetNextPVWidget();
    vtkPVLabeledToggle* tog = vtkPVLabeledToggle::SafeDownCast(pvw);
    if (tog)
      {
      tog->Disable();
      }

    vtkPVSelectionList* list = vtkPVSelectionList::SafeDownCast(pvw);
    if (list)
      {
      list->Disable();
      }
    }

  int numOutputs = reader->GetNumberOfOutputs();
  int numPVOutputs = this->GetNumberOfPVOutputs();
  if ( this->Initialized )
    {
    if ( numOutputs < numPVOutputs )
      {
      vtkWarningMacro("Although there are now fewer outputs, the connection "
                      "points will not be removed. Some of the outputs "
                      "will be empty.");
      }
    }

  if (numOutputs > 1)
    {
    this->HideDisplayPageOn();
    this->HideInformationPageOn();
    }

  window->GetMainView()->DisableRenderingFlagOn();

  int begin=0;
  if ( this->Initialized && numOutputs > numPVOutputs )
    {
    begin = numPVOutputs;
    }

  if ( !this->Initialized || numOutputs > numPVOutputs )
    {
    vtkPVPassThrough* connection;
    vtkPVData* connectionOutput;
  
    for (i = begin; i < numOutputs; i++)
      {
      // ith output (PVData)
      // Create replacement outputs of proper type and replace the
      // current ones.
      int len;
      if ( i == 0 )
        {
        len = (int)(strlen(tclName)+ strlen("Output")) + 3;
        }
      else
        {
        len = (int)(strlen(tclName)+ strlen("Output")) + 
          static_cast<int>(log10(static_cast<double>(i))) + 3;
        }
      outputTclName = new char[len];
      sprintf(outputTclName, "%sOutput%d", tclName, i);
      d = static_cast<vtkDataSet*>(pvApp->MakeTclObject(
        reader->GetOutput(i)->GetClassName(), outputTclName));
      pvApp->BroadcastScript("%s ShallowCopy [%s GetOutput %d]",
                             outputTclName, tclName, i);
      pvApp->BroadcastScript("%s SetOutput %d %s", tclName, i, 
                             outputTclName);
  
      pvd = vtkPVData::New();
      pvd->SetPVApplication(pvApp);

      int LODRes = this->GetPVRenderView()->GetLODResolution();
      vtkPVPart *part = vtkPVPart::New();
      part->SetPVApplication(pvApp);
      part->SetVTKData(d, outputTclName);
      pvApp->BroadcastScript("%s SetNumberOfDivisions %d %d %d", 
                             part->GetLODDeciTclName(), 
                             LODRes, LODRes, LODRes); 
      pvApp->GetProcessModule()->InitializePVPartPartition(part);
      pvd->SetPVPart(part);
      part->Delete();

      this->SetPVOutput(i, pvd);
      
      if (numOutputs <= 1)
        {
        // I do not know where this extent translator was created.
        // Set the original source again.
        pvApp->BroadcastScript("%s SetOriginalSource %s",
                              this->ExtentTranslatorTclName, 
                              outputTclName);
        // Set the translator of the output.
        pvApp->BroadcastScript("%s SetExtentTranslator %s",
                               outputTclName, 
                               this->ExtentTranslatorTclName);
        }
      else
        {
        // ith connection point and it's output
        // These are dummy filters (vtkPassThroughFilter) which
        // simply shallow copy their input to their output.
        connection = vtkPVPassThrough::New();
        connection->SetLabelNoTrace(this->GetLabel());
        connection->SetOutputNumber(i);
        connection->SetParametersParent(
          window->GetMainView()->GetSourceParent());
        connection->SetApplication(pvApp);

        // Create an extent translator for each output.
        char translatorTclName[512];
        sprintf(translatorTclName, "%sTranslator", outputTclName);
        pvApp->BroadcastScript("vtkPVExtentTranslator %s", translatorTclName);
        // Hold onto name so it can be deleted.
        connection->SetExtentTranslatorTclName(translatorTclName);
        // Set the original source again.
        pvApp->BroadcastScript("%s SetOriginalSource %s",
                              this->ExtentTranslatorTclName, 
                              outputTclName);
        // Set the translator of the output.
        pvApp->BroadcastScript("%s SetExtentTranslator %s",
                               outputTclName, 
                               this->ExtentTranslatorTclName);


        int len = static_cast<int>(strlen(tclName))+ 2 + 
          static_cast<int>(log10(static_cast<double>(i+1))) + 3;
        connectionTclName = new char[len];
        sprintf(connectionTclName, "%s_%d", tclName, i+1);
        vtkSource* source = static_cast<vtkSource*>(
          pvApp->MakeTclObject("vtkPassThroughFilter", connectionTclName));
        connection->SetVTKSource(source, connectionTclName);
        connection->SetView(window->GetMainView());
        connection->SetName(connectionTclName);
        connection->HideParametersPageOn();
        connection->SetTraceInitialized(1);
        
        if ( i == 0 )
          {
          len = static_cast<int>(strlen(connectionTclName)+ strlen("Output")) + 3;
          }
        else
          {
          len = static_cast<int>(strlen(connectionTclName)+ strlen("Output")) + 
                static_cast<int>(log10(static_cast<double>(i))) + 3;
          }
        outputTclName = new char[len];
        sprintf(outputTclName, "%sOutput%d", connectionTclName, i);
        d = static_cast<vtkDataSet*>(pvApp->MakeTclObject(
          reader->GetOutput(i)->GetClassName(), outputTclName));
        
        connectionOutput = vtkPVData::New();
        connectionOutput->SetPVApplication(pvApp);

        vtkPVPart *part = vtkPVPart::New();
        part->SetPVApplication(pvApp);
        part->SetVTKData(d, outputTclName);
        pvApp->GetProcessModule()->InitializePVPartPartition(part);
        connectionOutput->SetPVPart(part);
        part->Delete();
        connection->SetPVOutput(connectionOutput);
        
        connection->CreateProperties();
        // Created is called only on the first output when Accept()
        // is invoked on the input. We need to call it for the others.
        // (if it is verified that the Display pages for these outputs
        // are not necessary, this can be removed)
        if ( i > 0 )
          {
          pvd->CreateProperties();
          }
        connection->SetPVInput(pvd);
        pvApp->BroadcastScript("%s SetOutput %s", connectionTclName, 
                               outputTclName);
        
        connectionOutput->Delete();
        delete [] outputTclName;
        delete [] connectionTclName;
        
        window->GetSourceList("Sources")->AddItem(connection);
        connection->UpdateParameterWidgets();
        connection->Accept(0);
        
        connection->Delete();
        }

      delete [] outputTclName;
      pvd->Delete();
      }
    if (numOutputs > 1)
      {
      this->GetPVOutput()->SetVisibilityInternal(0);
      }
    }

  if (!this->Initialized)
    {
    window->ResetCameraCallback();
    }
  window->GetMainView()->DisableRenderingFlagOff();


  this->Superclass::Accept(hideFlag, hideSource);
}

//----------------------------------------------------------------------------
void vtkPVPLOT3DReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
