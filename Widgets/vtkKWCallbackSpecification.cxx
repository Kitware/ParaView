/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCallbackSpecification.cxx
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
#include "vtkKWCallbackSpecification.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------

int vtkKWCallbackSpecificationCommand(ClientData cd, Tcl_Interp *interp,
				      int argc, char *argv[]);

vtkKWCallbackSpecification::vtkKWCallbackSpecification()
{
  this->EventString     = NULL;
  this->CommandString   = NULL;
  this->CalledObject    = NULL;
  this->Window          = NULL;
  this->NextCallback    = NULL;
  this->CommandFunction = vtkKWCallbackSpecificationCommand;
}

vtkKWCallbackSpecification::~vtkKWCallbackSpecification()
{
  this->SetNextCallback(NULL);
  delete [] this->CommandString;
  delete [] this->EventString;
}

vtkKWCallbackSpecification* vtkKWCallbackSpecification::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkObjectFactory::CreateInstance("vtkKWCallbackSpecification");
  if(ret)
    {
    return (vtkKWCallbackSpecification*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWCallbackSpecification;
}

