/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCheckButton.cxx
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
#include "vtkKWCheckButton.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWCheckButton* vtkKWCheckButton::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWCheckButton");
  if(ret)
    {
    return (vtkKWCheckButton*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWCheckButton;
}




int vtkKWCheckButton::GetState()
{
  vtkKWObject::Script(this->Application,"set %sValue",
		      this->GetWidgetName());
  
  return vtkKWObject::GetIntegerResult(this->Application);
}

void vtkKWCheckButton::SetState(int s)
{
  if (s)
    {
    vtkKWObject::Script(this->Application,"%s select",this->GetWidgetName());
    }
  else
    {
    vtkKWObject::Script(this->Application,"%s deselect",this->GetWidgetName());
    }
}


void vtkKWCheckButton::Create(vtkKWApplication *app, char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("CheckButton already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  vtkKWObject::Script(app,"checkbutton %s -variable %sValue %s",
		      wname,wname,args);
}

