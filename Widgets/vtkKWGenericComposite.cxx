/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWGenericComposite.cxx
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
#include "vtkKWGenericComposite.h"
#include "vtkKWWidget.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWGenericComposite* vtkKWGenericComposite::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWGenericComposite");
  if(ret)
    {
    return (vtkKWGenericComposite*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWGenericComposite;
}





int vtkKWGenericCompositeCommand(ClientData cd, Tcl_Interp *interp,
				int argc, char *argv[]);

vtkKWGenericComposite::vtkKWGenericComposite()
{
  this->CommandFunction = vtkKWGenericCompositeCommand;

  this->Prop = NULL;
}

vtkKWGenericComposite::~vtkKWGenericComposite()
{
  this->SetProp(NULL);
}
