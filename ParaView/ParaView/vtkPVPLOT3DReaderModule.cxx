/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPLOT3DReaderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPLOT3DReaderModule.h"

#include "vtkCollection.h"
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
#include "vtkPVWidgetProperty.h"
#include "vtkPVWindow.h"
#include "vtkSource.h"
#include "vtkStructuredGrid.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPLOT3DReaderModule);
vtkCxxRevisionMacro(vtkPVPLOT3DReaderModule, "1.18.2.1");

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
  int i;
  vtkPVWindow* window = this->GetPVWindow();

  this->UpdateVTKSourceParameters();
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  
  pm->GetStream() << vtkClientServerStream::Invoke << this->GetVTKSourceID(0) 
                  << "GetFileName" << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->GetVTKSourceID(0) 
                  << "CanReadBinaryFile" << vtkClientServerStream::LastResult
                  << vtkClientServerStream::End;
  pm->SendStreamToServerRoot();
  int canread = 0;
  if(!pm->GetLastServerResult().GetArgument(0,0,&canread))
    {
    vtkErrorMacro(<< "Faild to get server result.");
    return;
    }
  if(!canread)
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

  vtkPVWidgetProperty *pvwp = 0;
  this->WidgetProperties->InitTraversal();
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    pvwp =
      static_cast<vtkPVWidgetProperty*>(this->WidgetProperties->GetNextItemAsObject());
    vtkPVLabeledToggle* tog =
      vtkPVLabeledToggle::SafeDownCast(pvwp->GetWidget());
    if (tog)
      {
      tog->Disable();
      }

    vtkPVSelectionList* list =
      vtkPVSelectionList::SafeDownCast(pvwp->GetWidget());
    if (list)
      {
      list->Disable();
      }
    }
  this->Superclass::Accept(hideFlag, hideSource);

}

//----------------------------------------------------------------------------
void vtkPVPLOT3DReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
