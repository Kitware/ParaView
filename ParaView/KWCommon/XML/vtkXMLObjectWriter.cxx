/*=========================================================================

  Module:    vtkXMLObjectWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLObjectWriter.h"

#include "vtkObjectFactory.h"
#include "vtkString.h"
#include "vtkXMLUtilities.h"
#include "vtkXMLDataElement.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#endif

vtkCxxRevisionMacro(vtkXMLObjectWriter, "1.10");

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
