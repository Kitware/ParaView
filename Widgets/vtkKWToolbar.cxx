/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWToolbar.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWToolbar.h"

int vtkKWToolbarCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWToolbar::vtkKWToolbar()
{
  this->CommandFunction = vtkKWToolbarCommand;
  this->Height = 23;
  this->Expanding = 0;

  this->Bar1 = vtkKWWidget::New();
  this->Bar1->SetParent(this);
}

//----------------------------------------------------------------------------
vtkKWToolbar::~vtkKWToolbar()
{
  this->Bar1->Delete();
  this->Bar1 = NULL;
}


//------------------------------------------------------------------------------
vtkKWToolbar* vtkKWToolbar::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWToolbar");
  if(ret)
    {
    return (vtkKWToolbar*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWToolbar;
}



//----------------------------------------------------------------------------
void vtkKWToolbar::Create(vtkKWApplication *app)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("widget already created");
    return;
    }
  this->SetApplication(app);

  // create the main frame for this widget
  // How do you specify a height ???
  this->Script( "frame %s -height %d -relief raised -bd 1", 
                this->GetWidgetName(), this->Height);

  this->Bar1->Create(app, "frame", "-bd 1 -relief flat");
  this->Script("pack %s -side left -fill y -expand yes -padx 4 -pady 2 -ipadx 2 -ipady 2",
               this->Bar1->GetWidgetName());
  this->Script("bind %s <Configure> {%s ScheduleResize}",
               this->Bar1->GetWidgetName(), this->GetTclName());

}

//----------------------------------------------------------------------------
void vtkKWToolbar::ScheduleResize()
{  
  if (this->Expanding)
    {
    return;
    }
  this->Expanding = 1;
  this->Script("after idle {%s Resize}", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkKWToolbar::Resize()
{
  this->Script("%s configure -height %d",
               this->Bar1->GetWidgetName(), this->Height);
  this->Expanding = 0;
}

