/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWObject.cxx
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
#include "vtkTclUtil.h"
#include "vtkKWApplication.h"
#include "vtkKWObject.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWObject* vtkKWObject::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWObject");
  if(ret)
    {
    return (vtkKWObject*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWObject;
}




int vtkKWObjectCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

vtkKWObject::vtkKWObject()
{
  this->TclName = NULL;
  this->Application = NULL;  
  this->CommandFunction = vtkKWObjectCommand;
}

vtkKWObject::~vtkKWObject()
{
  if (this->TclName)
    {
    delete [] this->TclName;
    }
}


void vtkKWObject::Serialize(ostream& os, vtkIndent indent)
{
  os << this->GetClassName() << endl;
  os << indent << "  {\n";
  this->SerializeSelf(os, indent.GetNextIndent());
  os << indent << "  }\n";
}

void vtkKWObject::Serialize(istream& is)
{
  char token[1024];
  // keep reading tokens until we find our token
  do 
    {    
    vtkKWSerializer::GetNextToken(&is,token);
    }
  while (strcmp(token,this->GetClassName()));
  vtkKWSerializer::ReadNextToken(&is,"{",this);

  // for each token process it
  do
    {
    vtkKWSerializer::GetNextToken(&is,token);
    if (token[0] == '{')
      {
      vtkKWSerializer::FindClosingBrace(&is,this);
      }
    else
      {
      this->SerializeToken(is,token);
      }
    }
  while (token[0] != '}');
}

const char *vtkKWObject::GetTclName()
{
  // is the name is already set the just return it
  if (this->TclName)
    {
    return this->TclName;
    }

  // otherwise we must register ourselves with tcl and get a name
  if (!this->Application)
    {
    vtkWarningMacro("attempt to create Tcl instance before application was set!");
    return NULL;
    }

  vtkTclGetObjectFromPointer(this->Application->GetMainInterp(), 
                             (void *)this, this->CommandFunction);
  this->TclName = 
    new char [strlen(this->Application->GetMainInterp()->result)+1];
  strcpy(this->TclName,this->Application->GetMainInterp()->result);
  return this->TclName;
}

void vtkKWObject::Script(vtkKWApplication *app, char *format, ...)
{
  static char event[16000];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  if (Tcl_GlobalEval(app->GetMainInterp(), event) != TCL_OK)
    {
    vtkGenericWarningMacro("Error returned from tcl script.\n" <<
			   app->GetMainInterp()->result << endl);
    }
}

int vtkKWObject::GetIntegerResult(vtkKWApplication *app)
{
  int res;
  
  res = atoi(Tcl_GetStringResult(app->GetMainInterp()));
  return res;
}
float vtkKWObject::GetFloatResult(vtkKWApplication *app)
{
  float res;
  
  res = atof(Tcl_GetStringResult(app->GetMainInterp()));
  return res;
}

