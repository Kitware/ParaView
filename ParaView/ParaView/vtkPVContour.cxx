/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContour.cxx
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
#include "vtkPVContour.h"
#include "vtkPVInputMenu.h"
#include "vtkPVScalarRangeLabel.h"
#include "vtkPVContourEntry.h"
#include "vtkPVLabeledToggle.h"
#include "vtkPVApplication.h"
#include "vtkPVSourceInterface.h"
#include "vtkContourFilter.h"
#include "vtkPVData.h"
#include "vtkPVApplication.h"
#include "vtkKWMessageDialog.h"
#include "vtkPVWindow.h"
#include "vtkKWCompositeCollection.h"
#include "vtkObjectFactory.h"

int vtkPVContourCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVContour::vtkPVContour()
{
  this->CommandFunction = vtkPVContourCommand;
  
  this->ArrayMenu = NULL;

  this->ReplaceInputOff();
}

//----------------------------------------------------------------------------
vtkPVContour::~vtkPVContour()
{
  if (this->ArrayMenu)
    {
    this->ArrayMenu->UnRegister(this);
    this->ArrayMenu = NULL;
    }
}

//----------------------------------------------------------------------------
vtkPVContour* vtkPVContour::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVContour");
  if(ret)
    {
    return (vtkPVContour*)ret;
    }
  return new vtkPVContour;
}

//----------------------------------------------------------------------------
void vtkPVContour::CreateProperties()
{
  vtkPVApplication*      pvApp = this->GetPVApplication();
  vtkPVArrayMenu*        arrayMenu;
  vtkPVContourEntry*     entry;
  vtkPVLabeledToggle*    computeScalarsCheck;
  vtkPVLabeledToggle*    computeNormalsCheck;
  vtkPVLabeledToggle*    computeGradientsCheck;

  this->vtkPVSource::CreateProperties();

  this->AddInputMenu("Input", "PVInput", "vtkDataSet",
                     "Set the input to this filter.",
                     this->GetPVWindow()->GetSources()); 
  

  arrayMenu = this->AddArrayMenu("Scalars", vtkDataSetAttributes::SCALARS, 1,
                                 "Choose which scalar array you want to contour.");

  // We need to keep this around to check for scalars when the input changes.
  this->ArrayMenu = arrayMenu;
  this->ArrayMenu->Register(this);

  this->AddScalarRangeLabel(arrayMenu);
  
  entry = vtkPVContourEntry::New();
  entry->SetPVSource(this);
  entry->SetParent(this->GetParameterFrame()->GetFrame());
  entry->SetLabel("Contour Values");
  entry->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  entry->Create(pvApp);
  this->AddPVWidget(entry);
  this->Script("pack %s", entry->GetWidgetName());
  entry->Delete();
  entry = NULL;
  
  computeNormalsCheck = vtkPVLabeledToggle::New();
  computeNormalsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  computeNormalsCheck->SetLabel("Compute Normals");
  computeNormalsCheck->SetObjectVariable(this->GetVTKSourceTclName(), 
                                         "ComputeNormals");
  computeNormalsCheck->SetModifiedCommand(this->GetTclName(), 
                                          "ChangeAcceptButtonColor");
  computeNormalsCheck->Create(pvApp, "Select whether to compute normals");
  computeNormalsCheck->SetState(1);
  this->AddPVWidget(computeNormalsCheck);

  computeGradientsCheck = vtkPVLabeledToggle::New();
  computeGradientsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  computeGradientsCheck->SetLabel("Compute Gradients");
  computeGradientsCheck->SetObjectVariable(this->GetVTKSourceTclName(), 
                                           "ComputeGradients");
  computeGradientsCheck->SetModifiedCommand(this->GetTclName(), 
                                            "ChangeAcceptButtonColor");
  computeGradientsCheck->Create(pvApp, "Select whether to compute gradients");
  computeGradientsCheck->SetState(0);
  this->AddPVWidget(computeGradientsCheck);
  
  computeScalarsCheck = vtkPVLabeledToggle::New();
  computeScalarsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  computeScalarsCheck->SetLabel("Compute Scalars");
  computeScalarsCheck->SetObjectVariable(this->GetVTKSourceTclName(), 
                                         "ComputeScalars");
  computeScalarsCheck->SetModifiedCommand(this->GetTclName(), 
                                          "ChangeAcceptButtonColor");
  computeScalarsCheck->Create(pvApp, "Select whether to compute scalars");
  computeScalarsCheck->SetState(1);
  this->AddPVWidget(computeScalarsCheck);

  this->Script("pack %s %s %s -anchor w -padx 10",
               computeNormalsCheck->GetWidgetName(),
               computeGradientsCheck->GetWidgetName(),
               computeScalarsCheck->GetWidgetName());

  computeNormalsCheck->Delete();
  computeNormalsCheck= NULL;
  computeGradientsCheck->Delete();
  computeGradientsCheck= NULL;
  computeScalarsCheck->Delete();
  computeScalarsCheck= NULL;

  this->UpdateParameterWidgets();
}


//----------------------------------------------------------------------------
void vtkPVContour::SetPVInput(vtkPVData *input)
{
  if (input == this->GetPVInput())
    {
    return;
    }
  this->vtkPVSource::SetPVInput(input);

  if (this->ArrayMenu == NULL)
    {
    vtkErrorMacro("Please set the input after you create the properties.  We need to check for scalars.");
    return;
    }

  this->ArrayMenu->Reset();
  if (this->ArrayMenu->GetValue() == NULL)
    {
    vtkKWMessageDialog::PopupMessage(this->Application, 
				     this->Application->GetMainWindow(),
				     vtkKWMessageDialog::Warning, "Warning", 
				     "Input does not have scalars to contour.");
    }
}


//----------------------------------------------------------------------------
void vtkPVContour::SaveInTclScript(ofstream* file)
{
  char* tempName;
  int i;
  vtkContourFilter *source =
    (vtkContourFilter*)this->GetVTKSource();
  vtkPVSourceInterface *pvsInterface =
    this->GetPVInput()->GetPVSource()->GetInterface();
  

  *file << this->VTKSource->GetClassName() << " "
        << this->VTKSourceTclName << "\n";

  *file << "\t" << this->VTKSourceTclName << " SetInput [";

  if (!this->DefaultScalarsName)
    {
    if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                               "vtkGenericEnSightReader") == 0)
      {
      char *charFound;
      int pos;
      char *dataName = new char[strlen(this->GetPVInput()->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetPVInput()->GetVTKDataTclName());
      
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      delete [] dataName;
      }
    else if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                                    "vtkPDataSetReader") == 0)
      {
      char *dataName = new char[strlen(this->GetPVInput()->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetPVInput()->GetVTKDataTclName());
      
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput]\n\t";
      delete [] dataName;
      }
    else
      {
      *file << this->GetPVInput()->GetPVSource()->GetVTKSourceTclName()
            << " GetOutput]\n\t";
      }
    }
  
  *file << this->VTKSourceTclName << " SetNumberOfContours "
        << source->GetNumberOfContours() << "\n\t";
  
  for (i = 0; i < source->GetNumberOfContours(); i++)
    {
    *file << this->VTKSourceTclName << " SetValue " << i << " "
          << source->GetValue(i) << "\n\t";
    }

  this->GetPVOutput(0)->SaveInTclScript(file, this->VTKSourceTclName);
}
