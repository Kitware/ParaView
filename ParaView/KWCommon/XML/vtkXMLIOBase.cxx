/*=========================================================================

  Module:    vtkXMLIOBase.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLIOBase.h"

#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLUtilities.h"

vtkCxxRevisionMacro(vtkXMLIOBase, "1.2");

vtkCxxSetObjectMacro(vtkXMLIOBase, Object, vtkObject);

static int vtkXMLIOBaseDefaultCharacterEncoding = VTK_ENCODING_UTF_8;

//----------------------------------------------------------------------------
vtkXMLIOBase::vtkXMLIOBase()
{
  this->Object = 0;
  this->ErrorLog = NULL;
}

//----------------------------------------------------------------------------
vtkXMLIOBase::~vtkXMLIOBase()
{
  this->SetObject(0);
  this->SetErrorLog(NULL);
}

//----------------------------------------------------------------------------
void vtkXMLIOBase::SetDefaultCharacterEncoding(int val)
{
  if (val == vtkXMLIOBaseDefaultCharacterEncoding)
    {
    return;
    }

  if (val < VTK_ENCODING_NONE)
    {
    val = VTK_ENCODING_NONE;
    }
  else if (val > VTK_ENCODING_UNKNOWN)
    {
    val = VTK_ENCODING_UNKNOWN;
    }

  vtkXMLIOBaseDefaultCharacterEncoding = val;
}

//----------------------------------------------------------------------------
int vtkXMLIOBase::GetDefaultCharacterEncoding()
{
  return vtkXMLIOBaseDefaultCharacterEncoding;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLIOBase::NewDataElement()
{
  vtkXMLDataElement *elem = vtkXMLDataElement::New();
  elem->SetAttributeEncoding(vtkXMLIOBaseDefaultCharacterEncoding);
  return elem;
}

//----------------------------------------------------------------------------
void vtkXMLIOBase::AppendToErrorLog(const char *msg)
{
  ostrstream str;
  if (this->ErrorLog)
    {
    str << this->ErrorLog << endl;
    }
  str << msg << ends;
  this->SetErrorLog(str.str());
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkXMLIOBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->Object)
    {
    os << indent << "Object: " << this->Object << "\n";
    }
  else
    {
    os << indent << "Object: (none)\n";
    }

  os << indent << "ErrorLog: " 
     << (this->ErrorLog ? this->ErrorLog : "(none)") << endl;
}
