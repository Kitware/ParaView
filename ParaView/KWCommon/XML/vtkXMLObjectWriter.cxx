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
#include "vtkXMLObjectWriter.h"

#include "vtkObjectFactory.h"
#include "vtkString.h"
#include "vtkXMLUtilities.h"
#include "vtkXMLDataElement.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#endif

vtkCxxRevisionMacro(vtkXMLObjectWriter, "1.9");

//----------------------------------------------------------------------------
vtkXMLObjectWriter::vtkXMLObjectWriter()
{
  this->WriteFactored = 1;
  this->WriteIndented = 0;
}

//----------------------------------------------------------------------------
int vtkXMLObjectWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!elem)
    {
    return 0;
    }

  // Add revisions (actually output only the last rev number)

  strstream revisions;
  this->CollectRevisions(revisions);
  revisions << ends;

  const char *ptr = vtkString::FindLastString(revisions.str(), "$Revision: ");
  if (ptr)
    {
    char buffer[256];
    strcpy(buffer, ptr + strlen("$Revision: "));
    buffer[strlen(buffer) - 3] = '\0';
    elem->SetAttribute("Version", buffer);
    }
  revisions.rdbuf()->freeze(0);

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLObjectWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!elem)
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLObjectWriter::Create(vtkXMLDataElement *elem)
{
  if (!elem)
    {
    return 0;
    }

  elem->SetName(this->GetRootElementName());
  this->AddAttributes(elem);
  this->AddNestedElements(elem);

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLObjectWriter::CreateInElement(vtkXMLDataElement *parent)
{
  if (!parent)
    {
    return 0;
    }

  // Don't bother inserting if we can't create the final element

  vtkXMLDataElement *nested_elem = this->NewDataElement();
  if (!this->Create(nested_elem))
    {
    nested_elem->Delete();
    return 0;
    }

  // Insert the element

  parent->AddNestedElement(nested_elem);
  nested_elem->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLObjectWriter::CreateInNestedElement(vtkXMLDataElement *grandparent, 
                                              const char *name)
{
  if (!grandparent || !name || !*name)
    {
    return 0;
    }

  // Create the first nested element (parent)

  vtkXMLDataElement *parent = this->NewDataElement();

  // Create and insert the element inside the parent

  if (!this->CreateInElement(parent))
    {
    parent->Delete();
    return 0;
    }

  // Set the parent name and insert it into grandparent

  parent->SetName(name);
  grandparent->AddNestedElement(parent);
  parent->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLObjectWriter::WriteToStream(ostream &os, vtkIndent *indent)
{
  // Create the element

  vtkXMLDataElement *elem = this->NewDataElement();
  this->Create(elem);

  // Factor it

  if (this->WriteFactored)
    {
    vtkXMLUtilities::FactorElements(elem);
    }

  // Output the element

  vtkIndent internal_indent;
  if (this->WriteIndented)
    {
    if (!indent)
      {
      indent = &internal_indent;
      }
    }
  else
    {
    indent = 0;
    }

  vtkXMLUtilities::FlattenElement(elem, os, indent);

  elem->Delete();

  os.flush();
  if (os.fail())
    {
    return 0;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLObjectWriter::WriteToFile(const char *filename)
{
  ofstream os(filename, ios::out);
  int ret = this->WriteToStream(os);
  
  if (!ret)
    {
    os.close();
    unlink(filename);
    }
  
  return ret;
}

//----------------------------------------------------------------------------
void vtkXMLObjectWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "WriteFactored: " 
     << (this->WriteFactored ? "On" : "Off") << endl;

  os << indent << "WriteIndented: " 
     << (this->WriteIndented ? "On" : "Off") << endl;
}
