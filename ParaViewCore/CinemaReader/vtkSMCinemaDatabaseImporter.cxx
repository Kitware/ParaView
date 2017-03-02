/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCinemaDatabaseImporter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCinemaDatabaseImporter.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVCinemaDatabaseInformation.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSelfGeneratingSourceProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <map>
#include <sstream>

vtkObjectFactoryNewMacro(vtkSMCinemaDatabaseImporter);
//----------------------------------------------------------------------------
vtkSMCinemaDatabaseImporter::vtkSMCinemaDatabaseImporter()
{
}

//----------------------------------------------------------------------------
vtkSMCinemaDatabaseImporter::~vtkSMCinemaDatabaseImporter()
{
}

//----------------------------------------------------------------------------
bool vtkSMCinemaDatabaseImporter::SupportsCinema(vtkSMSession* session)
{
  return (session != NULL &&
    session->GetNumberOfProcesses(vtkPVSession::DATA_SERVER | vtkPVSession::RENDER_SERVER) == 1 &&
    session->GetRenderClientMode() == vtkSMSession::RENDERING_UNIFIED);
}

//----------------------------------------------------------------------------
bool vtkSMCinemaDatabaseImporter::ImportCinema(
  const std::string& dbase, vtkSMSession* session, vtkSMViewProxy* view)
{
  if (!this->SupportsCinema(session) || dbase.empty())
  {
    return false;
  }

  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  vtkSmartPointer<vtkSMProxy> cdbase;
  cdbase.TakeReference(pxm->NewProxy("misc", "CinemaDatabase"));
  if (!cdbase)
  {
    vtkErrorMacro("Failed to create necessary reader. Cannot import Cinema database!");
    return false;
  }

  SM_SCOPED_TRACE(CallFunction)
    .arg("ImportCinema")
    .arg("filename", dbase.c_str())
    .arg("view", view)
    .arg("comment", "import cinema database");

  vtkSMPropertyHelper(cdbase, "FileName").Set(dbase.c_str());
  cdbase->UpdateVTKObjects();

  // now check how many pipeline objects are there in this database.
  vtkNew<vtkPVCinemaDatabaseInformation> cinemaInfo;
  cdbase->GatherInformation(cinemaInfo.Get());

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  bool update_scene = false;

  std::map<std::string, vtkSmartPointer<vtkSMProxy> > readerProxies;
  for (size_t cc = 0; cc < cinemaInfo->GetPipelineObjects().size(); ++cc)
  {
    const std::string objectname = cinemaInfo->GetPipelineObjects()[cc];
    const vtkPVCinemaDatabaseInformation::VectorOfStrings& parentNames =
      cinemaInfo->GetPipelineObjectParents(objectname);

    vtkSmartPointer<vtkSMProxy> reader;
    if (parentNames.size() == 0)
    {
      reader.TakeReference(pxm->NewProxy("sources", "CinemaDatabasePipelineObjectReader"));
    }
    else
    {
      reader.TakeReference(pxm->NewProxy("sources", "CinemaDatabasePipelineObjectFilter"));
    }
    if (!reader)
    {
      continue;
    }
    readerProxies[objectname] = reader;

    controller->PreInitializeProxy(reader);
    vtkSMPropertyHelper(reader, "FileName").Set(dbase.c_str());
    vtkSMPropertyHelper(reader, "PipelineObject").Set(objectname.c_str());

    // hookup inputs. `cinemareader.py` ensures that the list of objects is
    // sorted so we can find all our inputs.
    for (vtkPVCinemaDatabaseInformation::VectorOfStrings::const_iterator pniter =
           parentNames.begin();
         pniter != parentNames.end(); ++pniter)
    {
      assert(readerProxies.find(*pniter) != readerProxies.end());
      vtkSMPropertyHelper(reader, "Input").Add(readerProxies[*pniter]);
    }
    reader->SetAnnotation("CINEMA", "1");
    reader->UpdateVTKObjects();

    // Now let's add runtime properties, if needed.
    this->AddPropertiesForControls(
      vtkSMSelfGeneratingSourceProxy::SafeDownCast(reader), objectname, cinemaInfo.Get());

    controller->PostInitializeProxy(reader);
    controller->RegisterPipelineProxy(reader, objectname.c_str());

    // Show in view, if provided.
    if (view && cinemaInfo->GetPipelineObjectVisibility(objectname))
    {
      controller->Show(vtkSMSourceProxy::SafeDownCast(reader), 0, view);
    }
    update_scene = true;
  }

  if (update_scene)
  {
    vtkSMProxy* scene = controller->GetAnimationScene(session);
    vtkSMAnimationSceneProxy::UpdateAnimationUsingDataTimeSteps(scene);
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMCinemaDatabaseImporter::AddPropertiesForControls(vtkSMSelfGeneratingSourceProxy* reader,
  const std::string& objectname, const vtkPVCinemaDatabaseInformation* cinfo)
{
  const vtkPVCinemaDatabaseInformation::VectorOfStrings& controlParams =
    cinfo->GetControlParameters(objectname);

  if (reader == NULL || controlParams.size() == 0)
  {
    // no control parameters available for this parameter. Nothing to do.
    return;
  }

  std::ostringstream str;
  str << "<Proxy>";
  for (vtkPVCinemaDatabaseInformation::VectorOfStrings::const_iterator iter = controlParams.begin();
       iter != controlParams.end(); ++iter)
  {
    // We may want to revisit this. For now, I am just putting everything in an
    // enumeration domain since that makes it easier given what we already
    // have.
    const vtkPVCinemaDatabaseInformation::VectorOfStrings& values =
      cinfo->GetControlParameterValues(*iter);
    str << "<IntVectorProperty name='" << iter->c_str() << "'          "
        << "                   initial_string='" << iter->c_str() << "'"
        << "                   clean_command='ClearControlParameter'   "
        << "                   command='EnableControlParameterValue'   "
        << "                   repeat_command='1'                      "
        << "                   number_of_elements_per_command='1' >    "
        << " <EnumerationDomain name='enum'>                           ";
    for (size_t cc = 0; cc < values.size(); ++cc)
    {
      str << "   <Entry value='" << cc << "' text='" << values[cc].c_str() << "' />";
    }
    str << " </EnumerationDomain>                                    "
        << "</IntVectorProperty>";
  }
  str << "</Proxy>";
  reader->ExtendDefinition(str.str().c_str());
}
//----------------------------------------------------------------------------
void vtkSMCinemaDatabaseImporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
