/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWEntry.cxx
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
#include "vtkKWEntry.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWEntry* vtkKWEntry::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWEntry");
  if(ret)
    {
    return (vtkKWEntry*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWEntry;
}




char *vtkKWEntry::GetValue()
{
  vtkKWObject::Script(this->Application,
		      "%s get", this->GetWidgetName());
  return this->Application->GetMainInterp()->result;
}

int vtkKWEntry::GetValueAsInt()
{
  return atoi(this->GetValue());
}

float vtkKWEntry::GetValueAsFloat()
{
  return atof(this->GetValue());
}

void vtkKWEntry::SetValue(char *s)
{
  vtkKWObject::Script(this->Application,"%s delete 0 end", 
		      this->GetWidgetName());
  if (s)
    {
    vtkKWObject::Script(this->Application,"%s insert 0 {%s}", 
			this->GetWidgetName(),s);
    }
}

void vtkKWEntry::SetValue(int i)
{
  vtkKWObject::Script(this->Application,"%s delete 0 end", 
		      this->GetWidgetName());
  vtkKWObject::Script(this->Application,"%s insert 0 %d", 
			this->GetWidgetName(),i);
}

void vtkKWEntry::SetValue(float f, int size)
{
  char tmp[1024];
  
  vtkKWObject::Script(this->Application,"%s delete 0 end", 
		      this->GetWidgetName());
  sprintf(tmp,"%%s insert 0 %%.%df",size);
  vtkKWObject::Script(this->Application,tmp,this->GetWidgetName(),f);
}

void vtkKWEntry::Create(vtkKWApplication *app, char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Entry already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  vtkKWObject::Script(app,"entry %s -textvariable %sValue %s",
		      wname,wname,args);
}

