/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMaterialLoaderProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMaterialLoaderProxy.h"

#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"


#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkSMMaterialLoaderProxy);
vtkCxxSetObjectMacro(vtkSMMaterialLoaderProxy, PropertyProxy, vtkSMProxy);

//-----------------------------------------------------------------------------
vtkSMMaterialLoaderProxy::vtkSMMaterialLoaderProxy()
{
  this->PropertyProxy = 0;
}

//-----------------------------------------------------------------------------
vtkSMMaterialLoaderProxy::~vtkSMMaterialLoaderProxy()
{
  this->SetPropertyProxy(0);
}

//-----------------------------------------------------------------------------
void vtkSMMaterialLoaderProxy::LoadMaterial(const char* materialname)
{
  if (!this->PropertyProxy)
    {
    if (materialname && materialname[0])
      {
      vtkErrorMacro("PropertyProxy must be set before LoadMaterial().");
      }
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  int send_contents = 0;
  char* xml = 0; 
 
  // When not in client mode, there is no server, hence, why bother sending the
  // xml file contents at all?
  if (materialname && strlen(materialname) > 0 && pm->GetOptions()->GetClientMode())
    {
    if (vtksys::SystemTools::FileExists(materialname))
      {
      ifstream fp;
      fp.open(materialname, ios::binary);
      if (!fp)
        {
        // failed to open file
        }
      else
        {
        // get length of file.
        fp.seekg(0, ios::end);
        unsigned int length = fp.tellg();
        fp.seekg(0, ios::beg);
        if (length > 0)
          {
          send_contents =1;
          xml = new char[length+1];
          fp.read(xml, length);
          xml[length] = 0;
          }
        fp.close();
        }
      }
    }
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->PropertyProxy->GetID();
  if (send_contents)
    {
    stream << "LoadMaterialFromString" << xml;
    }
  else
    {
    stream << "LoadMaterial" << (materialname? materialname : 0);
    }
  stream << vtkClientServerStream::End;
  pm->SendStream(this->PropertyProxy->GetConnectionID(),
                 this->PropertyProxy->GetServers(), stream);
  delete [] xml;
}

//-----------------------------------------------------------------------------
void vtkSMMaterialLoaderProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PropertyProxy: " << this->PropertyProxy << endl;
}
