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
vtkCxxRevisionMacro(vtkPVPLOT3DReaderModule, "1.15.4.2");

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
  pm->SendStreamToServer();
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

    vtkPVSelectionList* list = vtkPVSelectionList::SafeDownCast(pvwp);
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
