/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVErrorLogDisplay.cxx
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
#include "vtkKWApplication.h"
#include "vtkPVErrorLogDisplay.h"
#include "vtkKWPushButton.h"
#include "vtkKWText.h"
#include "vtkTimerLog.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWCheckButton.h"
#include "vtkVector.txx"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVErrorLogDisplay );

int vtkPVErrorLogDisplayCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVErrorLogDisplay::vtkPVErrorLogDisplay()
{
  this->ErrorMessages = 0;
}

//----------------------------------------------------------------------------
vtkPVErrorLogDisplay::~vtkPVErrorLogDisplay()
{
  if ( this->ErrorMessages )
    {
    this->ErrorMessages->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVErrorLogDisplay::AppendError(const char* msg)
{
  if ( !this->ErrorMessages )
    {
    this->ErrorMessages = vtkVector<const char*>::New();
    }
  this->ErrorMessages->AppendItem(msg);
}

//----------------------------------------------------------------------------
void vtkPVErrorLogDisplay::Clear()
{
  if ( this->ErrorMessages )
    {
    this->ErrorMessages->RemoveAllItems();
    }
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVErrorLogDisplay::Create(vtkKWApplication *app)
{
  this->Superclass::Create(app);
  this->Script("pack forget  %s %s %s %s",
               this->ThresholdLabel->GetWidgetName(),
               this->ThresholdMenu->GetWidgetName(),
	       this->EnableLabel->GetWidgetName(),
	       this->EnableCheck->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVErrorLogDisplay::Update()
{
  int cc;
  this->EnableWrite();
  this->DisplayText->SetValue("");
  if ( this->ErrorMessages )
    {
    for ( cc = 0; cc < this->ErrorMessages->GetNumberOfItems(); cc ++ )
      {
      const char* item = 0;
      if ( this->ErrorMessages->GetItem(cc, item) == VTK_OK && item )
	{
	this->Append(item);
	}
      }
    }
  else
    {
    this->DisplayText->SetValue("");
    this->Append("No errors");
    }
  this->DisableWrite();
}


//----------------------------------------------------------------------------
void vtkPVErrorLogDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << endl;
  os << indent << "Threshold: " << this->Threshold << endl;
}
