/*=========================================================================

  Program:   ParaView
  Module:    vtkKWGenericRenderWindowInteractor.cxx
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
#include "vtkKWGenericRenderWindowInteractor.h"

#include "vtkKWRenderWidget.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWGenericRenderWindowInteractor);
vtkCxxRevisionMacro(vtkKWGenericRenderWindowInteractor, "1.2");

//----------------------------------------------------------------------------
vtkKWGenericRenderWindowInteractor::vtkKWGenericRenderWindowInteractor()
{
  this->View = NULL;
  this->RenderWidget = NULL;
}

//----------------------------------------------------------------------------
vtkKWGenericRenderWindowInteractor::~vtkKWGenericRenderWindowInteractor()
{
  this->SetView(NULL);
  this->SetRenderWidget(NULL);
}

//----------------------------------------------------------------------------
void vtkKWGenericRenderWindowInteractor::SetView(vtkKWView *view)
{
  if (this->View != view)
    {
    // to avoid circular references
    this->View = view;
    if (this->View != NULL)
      {
      this->SetRenderWindow(this->View->GetRenderWindow());
      }
    else
      {
      this->SetRenderWindow(NULL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWGenericRenderWindowInteractor::SetRenderWidget(
  vtkKWRenderWidget *widget)
{
  if (this->RenderWidget != widget)
    {
    // to avoid circular references
    this->RenderWidget = widget;
    if (this->RenderWidget != NULL)
      {
      this->SetRenderWindow(this->RenderWidget->GetRenderWindow());
      }
    else
      {
      this->SetRenderWindow(NULL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWGenericRenderWindowInteractor::SetMoveEventInformationFlipY(
  int x, int y)
{
  this->SetEventInformationFlipY(x, y, this->ControlKey, this->ShiftKey,
                                 this->KeyCode, this->RepeatCount,
                                 this->KeySym);
}

//----------------------------------------------------------------------------
void vtkKWGenericRenderWindowInteractor::Render()
{
  if (this->View)
    {
    this->View->Render();
    }
  else if (this->RenderWidget)
    {
    this->RenderWidget->Render();
    }
}

//----------------------------------------------------------------------------
void vtkKWGenericRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "View: " << this->GetView() << endl;
  os << indent << "LastEventPosition: (" << this->LastEventPosition[0] << ", "
     << this->LastEventPosition[1] << ", " << this->LastEventPosition[2]
     << ")" << endl;
  os << indent << "RenderWidget: " << this->RenderWidget << endl;
}
