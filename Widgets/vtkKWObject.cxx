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
  this->NumberOfVersions = 0;
  this->Versions = NULL;
}

vtkKWObject::~vtkKWObject()
{
  if (this->TclName)
    {
    delete [] this->TclName;
    }
  for (int i = 0; i < this->NumberOfVersions; i++)
    {
    delete [] this->Versions[i*2];
    delete [] this->Versions[i*2+1];    
    }
  if (this->Versions)
    {
    delete [] this->Versions;
    }
}


void vtkKWObject::Serialize(ostream& os, vtkIndent indent)
{
  os << this->GetClassName() << endl;
  os << indent << "  {\n";
  this->SerializeRevision(os, indent.GetNextIndent());
  this->SerializeSelf(os, indent.GetNextIndent());
  os << indent << "  }\n";
}

void vtkKWObject::ExtractRevision(ostream& os,const char *revIn)
{
  char rev[128];
  sscanf(revIn,"$Revision: %s",rev);
  os << rev << endl;
}

void vtkKWObject::SerializeRevision(ostream& os, vtkIndent indent)
{
  os << indent << "vtkKWObject ";
  this->ExtractRevision(os,"$Revision: 1.4 $");
}

void vtkKWObject::Serialize(istream& is)
{
  char token[1024];
  // get the class name
  vtkKWSerializer::GetNextToken(&is,token);
  if (strcmp(token,this->GetClassName()))
    {
    vtkDebugMacro("A class name mismatch has occured.");
    }
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

void vtkKWObject::SerializeToken(istream& is, const char token[1024])
{
  char tmp[1024];
  
  if (!strcmp(token,"vtkKWObject"))
    {
    vtkKWSerializer::GetNextToken(&is,tmp);
    this->AddVersion("vtkKWObject",tmp);
    return;
    }
}

const char *vtkKWObject::GetTclName()
{
  // is the name is already set the just return it
  if (this->TclName)
    {
    return this->TclName;
    }

  // otherwise we must register ourselves with tcl and get a name
  if (!this->GetApplication())
    {
    vtkWarningMacro("attempt to create Tcl instance before application was set!");
    return NULL;
    }

  vtkTclGetObjectFromPointer(this->GetApplication()->GetMainInterp(), 
                             (void *)this, this->CommandFunction);
  this->TclName = 
    new char [strlen(this->GetApplication()->GetMainInterp()->result)+1];
  strcpy(this->TclName,this->GetApplication()->GetMainInterp()->result);
  return this->TclName;
}

void vtkKWObject::Script(char *format, ...)
{
  static char event[16000];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  if (this->GetApplication())
    {
    this->GetApplication()->SimpleScript(event);
    }
  else
    {
    vtkWarningMacro("Attempt to script a command without a KWApplication");
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

void vtkKWObject::SetApplication (vtkKWApplication* arg)
{  
  vtkDebugMacro(<< this->GetClassName() 
  << " (" << this << "): setting " << "Application" " to " << arg ); 
  if (this->Application != arg) 
    { 
    if (this->Application != NULL) 
      { 
      this->Application->UnRegister(this); 
      }
    this->Application = arg;
    if (this->Application != NULL)
      {
      this->Application->Register(this);
      }
    this->Modified();
    }
}

void vtkKWObject::AddVersion(const char *cname, const char *version)
{
  // allocate more space
  int cnt;
  
  char **objs = new char *[2*(this->NumberOfVersions+1)];

  // copy the old to the new
  for (cnt = 0; cnt < this->NumberOfVersions*2; cnt++)
    {
    objs[cnt] = this->Versions[cnt];
    }
  if (this->Versions)
    {
    delete [] this->Versions;
    }
  this->Versions = objs;
  char *classname = new char [strlen(cname)+1];
  sprintf(classname,"%s",cname);
  this->Versions[this->NumberOfVersions*2] = classname;

  
  // compute the revision, strip out extra junk
  char *revision = new char [strlen(version)+1];
  sprintf(revision,"%s",version);
  this->Versions[this->NumberOfVersions*2+1] = revision;
  this->NumberOfVersions++;
}

const char *vtkKWObject::GetVersion(const char *cname)
{
  for (int i = 0; i < this->NumberOfVersions; i++)
    {
    if (!strcmp(this->Versions[i*2],cname))
      {
      return this->Versions[i*2];
      }
    }
  return NULL;
}
