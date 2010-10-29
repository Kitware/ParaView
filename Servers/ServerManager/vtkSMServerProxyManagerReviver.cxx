/*=========================================================================

  Program:   ParaView
  Module:    vtkSMServerProxyManagerReviver.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMServerProxyManagerReviver.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPQStateLoader.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyLocator.h"
#include "vtkPVOptions.h"

#include "vtkPVConfig.h" // for PARAVIEW_USE_ICE_T
#include "vtkToolkits.h" // for VTK_USE_MPI

#include <vtksys/RegularExpression.hxx>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMServerProxyManagerReviver);
//-----------------------------------------------------------------------------
vtkSMServerProxyManagerReviver::vtkSMServerProxyManagerReviver()
{
}

//-----------------------------------------------------------------------------
vtkSMServerProxyManagerReviver::~vtkSMServerProxyManagerReviver()
{
}

//-----------------------------------------------------------------------------
int vtkSMServerProxyManagerReviver::ReviveRemoteServerManager(vtkIdType cid)
{
  // This method has to do the following:
  // 1) Save revival state for the cid.
  // 2) Cleanup client side proxy manager objects for the cid.
  // 3) Send state to the server and ask it to revive the server side
  //    server manager.

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // 1) Save revival state for the cid.
  vtkPVXMLElement* root = pxm->SaveRevivalState(cid);

  vtkClientServerStream stream;

  // hide server side objects from every proxy except views/representations. 
  // That way view/representations are totally cleaned up, but for all other
  // proxies, the server side objects remain.
  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();
  iter->SetConnectionID(cid);
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkstd::string group = iter->GetGroup();
    vtkstd::string proxy_name = iter->GetKey();
    vtkSMProxy* proxy = iter->GetProxy();
    const char* xmlgroup = proxy? proxy->GetXMLGroup() : 0;
    if (proxy && (xmlgroup==0 || 
        (strcmp(xmlgroup, "representations") !=0 &&
         strcmp(xmlgroup, "views") !=0)))
      {
      proxy->SetServers(proxy->GetServers() & vtkProcessModule::CLIENT);
      }
    }
  iter->Delete();

  // 2) Cleanup client side proxy manager objects for the cid.
  // No need to do SendStreamToClientOnlyOn() this anymore since
  // CreateVTKObjects() is never called when
  // reviving proxies. In fact, doing this can cause issues since we have now
  // changed the StateLoader to call UpdateVTKObjects() as and when the proxies
  // are created and their state loaded completely.
  // pm->SendStreamToClientOnlyOn();
  pxm->UnRegisterProxies(cid);  
  // pm->SendStreamToClientOnlyOff();

  vtksys_ios::ostringstream xml_stream;
  root->PrintXML(xml_stream, vtkIndent());

  //ofstream file("/tmp/revive.xml");
  //root->PrintXML(file, vtkIndent());
  //file.close();

  root->Delete();

  vtkClientServerID id = pm->NewStreamObject("vtkSMServerProxyManagerReviver", stream);
  stream << vtkClientServerStream::Invoke
    << id 
    << "ReviveServerServerManager"
    << xml_stream.str().c_str()
    << id.ID
    << vtkClientServerStream::End;
  pm->SendStream(cid, vtkProcessModule::DATA_SERVER_ROOT, stream);

  int revival_state = 0;
  pm->GetLastResult(cid, vtkProcessModule::DATA_SERVER_ROOT).GetArgument(
    0, 0, &revival_state);
  if (!revival_state)
    {
    vtkErrorMacro("Failed to succesfully revive the server.");
    }
  pm->DeleteStreamObject(id, stream);
  pm->SendStream(cid, vtkProcessModule::DATA_SERVER_ROOT, stream);
  return revival_state;
}

//-----------------------------------------------------------------------------
int vtkSMServerProxyManagerReviver::ReviveServerServerManager(
  const char* xml_state, int max_id)
{
  vtkClientServerID id; 
  id.ID = max_id;

  vtkProcessModule::DebugLog(xml_state);

  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  if (!parser->Parse(xml_state))
    {
    parser->Delete();
    return 0;
    }

  vtkSMPQStateLoader* loader = vtkSMPQStateLoader::New();
  // The state is loaded on the self connection.
  loader->GetProxyLocator()->SetConnectionID(
    vtkProcessModuleConnectionManager::GetSelfConnectionID());
  loader->GetProxyLocator()->SetReviveProxies(1);

  // The process module on the client side keeps track of the unique IDs.
  // We need to synchornize the IDs on the client side, so that when new IDs
  // are assigned, the ones already used aren't reassigned.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->ReserveID(id);
  pm->GetOptions()->SetProcessType(vtkPVOptions::PVBATCH);
  //pm->SendStreamToClientOnlyOn();
  vtkProcessModule::DebugLog("Pre--FilterStateXML");
  this->FilterStateXML(parser->GetRootElement());
  vtkProcessModule::DebugLog("Post--FilterStateXML");
  
  int ret = loader->LoadState(parser->GetRootElement());
  vtkProcessModule::DebugLog("Post--LoadState");
  //pm->SendStreamToClientOnlyOff();
  
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkProcessModule::DebugLog("Pre--UpdateRegisteredProxies");
  pxm->UpdateRegisteredProxies(0);
  vtkProcessModule::DebugLog("Post--UpdateRegisteredProxies");

  loader->Delete();
  parser->Delete();
  return ret;
}

//-----------------------------------------------------------------------------
void vtkSMServerProxyManagerReviver::FilterStateXML(vtkPVXMLElement* root)
{
  
  unsigned int max = root->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < max; cc++)
    {
    vtkPVXMLElement* element = root->GetNestedElement(cc);
    if (element->GetName() && strcmp(element->GetName(), "Proxy")==0)
      {
      int remove_revival_state = 0;
      vtkstd::string group = element->GetAttribute("group")?
        element->GetAttribute("group"): "";
      if (group == "views" || group == "representations" || group == "animation")
        {
        // We create views/representations all over again on the server.
        remove_revival_state = 1;
        }

      if (remove_revival_state)
        {
        unsigned int children_max = element->GetNumberOfNestedElements();
        for (unsigned int i=0; i < children_max; i++)
          {
          vtkPVXMLElement* child = element->GetNestedElement(i);
          if (child->GetName() && strcmp(child->GetName(), "RevivalState")==0)
            {
            element->RemoveNestedElement(child);
            break;
            }
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMServerProxyManagerReviver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
