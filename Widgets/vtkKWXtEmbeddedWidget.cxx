/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWXtEmbeddedWidget.cxx
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
#include "vtkKWXtEmbeddedWidget.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWXtEmbeddedWidget* vtkKWXtEmbeddedWidget::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWXtEmbeddedWidget");
  if(ret)
    {
    return (vtkKWXtEmbeddedWidget*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWXtEmbeddedWidget;
}




int vtkKWXtEmbeddedWidgetCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

vtkKWXtEmbeddedWidget::vtkKWXtEmbeddedWidget()
{
  this->WindowId = NULL;
}

vtkKWXtEmbeddedWidget::~vtkKWXtEmbeddedWidget()
{

}

void vtkKWXtEmbeddedWidget::Display()
{
  // map the window
  this->Script("wm deiconify %s",this->GetWidgetName());
}


void vtkKWXtEmbeddedWidget::Create(vtkKWApplication *app, char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Dialog already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  const char* wname = this->GetWidgetName();
  if(this->WindowId)
    {
    this->Script("toplevel %s %s -use 0x%x",wname,args, this->WindowId);
    }
  else
    {
    this->Script("toplevel %s %s",wname,args); 
    this->Script("wm withdraw %s",wname);
    }
}
