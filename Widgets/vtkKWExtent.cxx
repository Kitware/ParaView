/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWExtent.cxx
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
#include "vtkKWExtent.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWExtent* vtkKWExtent::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWExtent");
  if(ret)
    {
    return (vtkKWExtent*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWExtent;
}




int vtkKWExtentCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

vtkKWExtent::vtkKWExtent()
{
  this->CommandFunction = vtkKWExtentCommand;
  this->Command = NULL;

  this->XMinScale = vtkKWScale::New();
  this->XMinScale->SetParent(this);
  this->XMaxScale = vtkKWScale::New();
  this->XMaxScale->SetParent(this);
  this->YMinScale = vtkKWScale::New();
  this->YMinScale->SetParent(this);
  this->YMaxScale = vtkKWScale::New();
  this->YMaxScale->SetParent(this);
  this->ZMinScale = vtkKWScale::New();
  this->ZMinScale->SetParent(this);
  this->ZMaxScale = vtkKWScale::New();
  this->ZMaxScale->SetParent(this);

  this->Extent[0] = 0;
  this->Extent[1] = 0;
  this->Extent[2] = 0;
  this->Extent[3] = 0;
  this->Extent[4] = 0;
  this->Extent[5] = 0;
}

vtkKWExtent::~vtkKWExtent()
{
  if (this->Command)
    {
    delete [] this->Command;
    }

  this->XMinScale->Delete();  
  this->XMaxScale->Delete();
  this->YMinScale->Delete();  
  this->YMaxScale->Delete();
  this->ZMinScale->Delete();  
  this->ZMaxScale->Delete();
}

void vtkKWExtent::Create(vtkKWApplication *app, char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Extent already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s %s", wname,args);

  this->XMinScale->Create(this->Application,"-length 190");
  this->XMinScale->SetCommand(this, "ExtentSelected");
  this->XMinScale->DisplayEntry();
  this->XMinScale->DisplayLabel("Minimum X");

  this->XMaxScale->Create(this->Application,"");
  this->XMaxScale->SetCommand(this, "ExtentSelected");
  this->XMaxScale->DisplayEntry();
  this->XMaxScale->DisplayLabel("Maximum X");

  this->YMinScale->Create(this->Application,"");
  this->YMinScale->SetCommand(this, "ExtentSelected");
  this->YMinScale->DisplayEntry();
  this->YMinScale->DisplayLabel("Minimum Y");

  this->YMaxScale->Create(this->Application,"");
  this->YMaxScale->SetCommand(this, "ExtentSelected");
  this->YMaxScale->DisplayEntry();
  this->YMaxScale->DisplayLabel("Maximum Y");

  this->ZMinScale->Create(this->Application,"");
  this->ZMinScale->SetCommand(this, "ExtentSelected");
  this->ZMinScale->DisplayEntry();
  this->ZMinScale->DisplayLabel("Minimum Z");

  this->ZMaxScale->Create(this->Application,"");
  this->ZMaxScale->SetCommand(this, "ExtentSelected");
  this->ZMaxScale->DisplayEntry();
  this->ZMaxScale->DisplayLabel("Maximum Z");

  this->Script(
    "pack %s %s %s %s %s %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
    this->XMinScale->GetWidgetName(),
    this->XMaxScale->GetWidgetName(),
    this->YMinScale->GetWidgetName(),
    this->YMaxScale->GetWidgetName(),
    this->ZMinScale->GetWidgetName(),
    this->ZMaxScale->GetWidgetName());
}


void vtkKWExtent::SetExtentRange(int *er)
{
  this->SetExtentRange(er[0],er[1],er[2],er[3],er[4],er[5]);
}
void vtkKWExtent::SetExtentRange(int x1, int x2, int y1, int y2, 
                                 int z1, int z2)
{
  this->XMinScale->SetRange(x1,x2);
  this->XMaxScale->SetRange(x1,x2);
  this->YMinScale->SetRange(y1,y2);
  this->YMaxScale->SetRange(y1,y2);
  this->ZMinScale->SetRange(z1,z2);
  this->ZMaxScale->SetRange(z1,z2);
  this->SetExtent(x1,x2,y1,y2,z1,z2);
}

void vtkKWExtent::SetExtent(int x1, int x2, int y1, int y2, int z1, int z2)
{
  if (this->Extent[0] == x1 &&
      this->Extent[1] == x2 &&
      this->Extent[2] == y1 &&
      this->Extent[3] == y2 &&
      this->Extent[4] == z1 &&
      this->Extent[5] == z2)
    {
    return;
    }

  this->XMinScale->SetValue(x1);
  this->XMaxScale->SetValue(x2);
  this->YMinScale->SetValue(y1);
  this->YMaxScale->SetValue(y2);
  this->ZMinScale->SetValue(z1);
  this->ZMaxScale->SetValue(z2);
}

void vtkKWExtent::SetExtent(int *er)
{
  this->SetExtent(er[0],er[1],er[2],er[3],er[4],er[5]);
}

void vtkKWExtent::ExtentSelected()
{
  // first check to see if anything changed.
  // Normally something should have changed, but 
  // on initialization this isn;t the case.
  if (this->Extent[0] == this->XMinScale->GetValue() &&
      this->Extent[1] == this->XMaxScale->GetValue() &&
      this->Extent[2] == this->YMinScale->GetValue() &&
      this->Extent[3] == this->YMaxScale->GetValue() &&
      this->Extent[4] == this->ZMinScale->GetValue() &&
      this->Extent[5] == this->ZMaxScale->GetValue())
    {
    return;
    }
  
  this->Extent[0] = this->XMinScale->GetValue();
  this->Extent[1] = this->XMaxScale->GetValue();
  this->Extent[2] = this->YMinScale->GetValue();
  this->Extent[3] = this->YMaxScale->GetValue();
  this->Extent[4] = this->ZMinScale->GetValue();
  this->Extent[5] = this->ZMaxScale->GetValue();
 
  // handle error conditions
  if (this->Extent[0] > this->Extent[1])
    {
    this->Extent[0] = this->Extent[1];
    this->XMinScale->SetValue(this->Extent[0]);    
    }
  if (this->Extent[2] > this->Extent[3])
    {
    this->Extent[2] = this->Extent[3];
    this->YMinScale->SetValue(this->Extent[2]);    
    }
  if (this->Extent[4] > this->Extent[5])
    {
    this->Extent[4] = this->Extent[5];
    this->ZMinScale->SetValue(this->Extent[4]);    
    }
  
  this->Script("eval %s",this->Command);
}


void vtkKWExtent::SetCommand(vtkKWObject* CalledObject, const char *CommandString)
{ 
  ostrstream command;
  command << CalledObject->GetTclName() << " " << CommandString << ends;
  this->Command = command.str();
}
