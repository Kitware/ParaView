/*=========================================================================

  Program:   ParaView
  Module:    vtkSIPVRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIPVRepresentationProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVCompositeRepresentation.h"
#include "vtkPVXMLElement.h"
#include "vtkSISourceProxy.h"
#include "vtkSelectionRepresentation.h"
#include "vtkSmartPointer.h"

#include <map>
#include <string>

#include <assert.h>

class vtkSIPVRepresentationProxy::vtkInternals
{
public:
  struct vtkValue
  {
    vtkSmartPointer<vtkSIProxy> SubProxy;
    std::string SubText;
  };

  typedef std::map<std::string, vtkValue> RepresentationProxiesType;
  RepresentationProxiesType RepresentationProxies;
};

vtkStandardNewMacro(vtkSIPVRepresentationProxy);
//----------------------------------------------------------------------------
vtkSIPVRepresentationProxy::vtkSIPVRepresentationProxy()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkSIPVRepresentationProxy::~vtkSIPVRepresentationProxy()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
bool vtkSIPVRepresentationProxy::ReadXMLAttributes(vtkPVXMLElement* element)
{
  vtkPVCompositeRepresentation* pvrepresentation =
    vtkPVCompositeRepresentation::SafeDownCast(this->GetVTKObject());

  // Pass on the selection-representation
  if (vtkSIProxy* siProxy = this->GetSubSIProxy("SelectionRepresentation"))
  {
    vtkSelectionRepresentation* selection =
      vtkSelectionRepresentation::SafeDownCast(siProxy->GetVTKObject());
    pvrepresentation->SetSelectionRepresentation(selection);
  }

  // Update internal data-structures for the types of representations provided
  // by this instance.

  // <RepresentationType subproxy="OutlineRepresentation" text="Outline"
  //    subtype="0"/>
  unsigned int numElements = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElements; cc++)
  {
    vtkPVXMLElement* child = element->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "RepresentationType") == 0)
    {
      const char* name = child->GetAttribute("subproxy");
      vtkSIProxy* subproxy = this->GetSubSIProxy(name);
      if (!subproxy)
      {
        vtkErrorMacro("Missing data representation subproxy '"
          << (name ? name : "<null>") << "' when processing RepresentationType element.");
        return false;
      }

      const char* text = child->GetAttribute("text");
      if (!text)
      {
        vtkErrorMacro("Missing required 'text' attribute on RepresentationType element");
        return false;
      }

      // Add each of the sub-representations to the composite representation.
      pvrepresentation->AddRepresentation(
        text, vtkPVDataRepresentation::SafeDownCast(subproxy->GetVTKObject()));

      //// read optional subtype.
      const char* sub_text = child->GetAttribute("subtype");
      this->Internals->RepresentationProxies[text].SubProxy = subproxy;
      if (sub_text)
      {
        this->Internals->RepresentationProxies[text].SubText = sub_text;
      }
    }
  }

  // We add observer to the vtkCompositeRepresentation so that every time it's
  // modified, we ensure that the representation type is up-to-date.
  vtkObject::SafeDownCast(this->GetVTKObject())
    ->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtkSIPVRepresentationProxy::OnVTKObjectModified);

  return this->Superclass::ReadXMLAttributes(element);
}

//----------------------------------------------------------------------------
void vtkSIPVRepresentationProxy::OnVTKObjectModified()
{
  vtkCompositeRepresentation* repr = vtkCompositeRepresentation::SafeDownCast(this->GetVTKObject());
  const char* key = repr->GetActiveRepresentationKey();
  vtkInternals::RepresentationProxiesType::iterator iter = key
    ? this->Internals->RepresentationProxies.find(key)
    : this->Internals->RepresentationProxies.end();
  if (iter != this->Internals->RepresentationProxies.end() && iter->second.SubText != "")
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << iter->second.SubProxy->GetVTKObject()
           << "SetRepresentation" << iter->second.SubText.c_str() << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
  }
}

//----------------------------------------------------------------------------
void vtkSIPVRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
void vtkSIPVRepresentationProxy::AboutToDelete()
{
  this->Internals->RepresentationProxies.clear();
  this->Superclass::AboutToDelete();
}
