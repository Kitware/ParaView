/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWFrame.cxx
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
#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWFrame );

vtkKWFrame::vtkKWFrame()
{
  this->ScrollFrame = 0;
  this->Frame = 0;
}

vtkKWFrame::~vtkKWFrame()
{
  if ( this->ScrollFrame )
    {
    this->ScrollFrame->Delete();
    }
  if (this->Frame && this->Frame != this)
    {
    this->Frame->Delete();
    }
}


void vtkKWFrame::Create(vtkKWApplication *app, int scrollable)
{
  const char *wname;
  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("ScrollableFrame already created");
    return;
    }

  this->SetApplication(app);

  
  if ( scrollable )
    {
    // create the top level
    wname = this->GetWidgetName();
    this->Script("ScrolledWindow %s -relief flat -borderwidth 2", wname);

    this->ScrollFrame = vtkKWWidget::New();

    this->ScrollFrame->SetParent(this);
    this->ScrollFrame->Create(this->Application, "ScrollableFrame", "-height 1024");
    this->Script("%s setwidget %s", this->GetWidgetName(),
		 this->ScrollFrame->GetWidgetName());

    this->Frame = vtkKWWidget::New();
    this->Frame->SetParent(this->ScrollFrame);
    this->Script("%s getframe", this->ScrollFrame->GetWidgetName());
    this->Frame->SetWidgetName(this->Application->GetMainInterp()->result);

    this->Script("%s configure -constrainedwidth 1", this->ScrollFrame->GetWidgetName());

    }
  else
    {
    // create the top level
    wname = this->GetWidgetName();
    this->Script("frame %s -borderwidth 2 -relief flat", wname);

    this->Frame = this;
    }
}


