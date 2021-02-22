/*=========================================================================

  Program:   ParaView
  Module:    vtkSIXMLAnimationWriterRepresentationProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIXMLAnimationWriterRepresentationProperty.h"

#include "vtkObjectFactory.h"
#include "vtkSIProxy.h"
#include "vtkSMMessage.h"

#include <sstream>

vtkStandardNewMacro(vtkSIXMLAnimationWriterRepresentationProperty);
//----------------------------------------------------------------------------
vtkSIXMLAnimationWriterRepresentationProperty::vtkSIXMLAnimationWriterRepresentationProperty() =
  default;

//----------------------------------------------------------------------------
vtkSIXMLAnimationWriterRepresentationProperty::~vtkSIXMLAnimationWriterRepresentationProperty() =
  default;

//----------------------------------------------------------------------------
bool vtkSIXMLAnimationWriterRepresentationProperty::Push(vtkSMMessage* message, int offset)
{
  assert(message->ExtensionSize(ProxyState::property) > offset);

  const ProxyState_Property prop = message->GetExtension(ProxyState::property, offset);
  assert(strcmp(prop.name().c_str(), this->GetXMLName()) == 0);

  const Variant variant = prop.value();
  std::vector<vtkTypeUInt32> proxy_ids;
  proxy_ids.resize(variant.proxy_global_id_size());
  for (int cc = 0; cc < variant.proxy_global_id_size(); cc++)
  {
    proxy_ids[cc] = variant.proxy_global_id(cc);
  }

  vtkObjectBase* object = this->GetVTKObject();
  vtkClientServerStream stream;
  if (this->CleanCommand)
  {
    stream << vtkClientServerStream::Invoke << object << this->CleanCommand
           << vtkClientServerStream::End;
  }
  for (size_t cc = 0; cc < proxy_ids.size(); cc++)
  {
    vtkSIProxy* siProxy = vtkSIProxy::SafeDownCast(this->GetSIObject(proxy_ids[cc]));

    // Assign unique group name for each source.
    std::ostringstream groupname_str;
    groupname_str << "source" << proxy_ids[cc];
    stream << vtkClientServerStream::Invoke << object << this->GetCommand()
           << siProxy->GetVTKObject() << groupname_str.str().c_str() << vtkClientServerStream::End;
  }
  return this->ProcessMessage(stream);
}

//----------------------------------------------------------------------------
void vtkSIXMLAnimationWriterRepresentationProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
