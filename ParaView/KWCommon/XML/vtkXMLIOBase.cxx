/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkXMLIOBase.h"

#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLUtilities.h"

vtkCxxRevisionMacro(vtkXMLIOBase, "1.1.4.1");

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
