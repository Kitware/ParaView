/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractsController.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExtractsController.h"

#include "vtkCollection.h"
#include "vtkCollectionRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkProcessModule.h"
#include "vtkRemoteWriterHelper.h"
#include "vtkSMExtractTriggerProxy.h"
#include "vtkSMExtractWriterProxy.h"
#include "vtkSMFileUtilities.h"
#include "vtkSMOutputPort.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

// clang-format off
#include "vtk_doubleconversion.h"
#include VTK_DOUBLECONVERSION_HEADER(double-conversion.h)
// clang-format on

#include <sstream>
#include <vtksys/SystemTools.hxx>

namespace
{
std::string ConvertToString(const double val)
{
  char buf[256];
  const double_conversion::DoubleToStringConverter& converter =
    double_conversion::DoubleToStringConverter::EcmaScriptConverter();
  double_conversion::StringBuilder builder(buf, sizeof(buf));
  builder.Reset();
  converter.ToShortest(val, &builder);
  return builder.Finalize();
}

std::string RelativePath(const std::string& root, const std::string& target)
{
  // vtksys::SystemTools::RelativePath() does not work unless both paths are
  // full paths; so make them full relative to CWD, if needed.
  return vtksys::SystemTools::RelativePath(
    vtksys::SystemTools::CollapseFullPath(root), vtksys::SystemTools::CollapseFullPath(target));
}

std::string DefaultFilenamePrefix(vtkSMProxy* input)
{
  auto pxm = input->GetSessionProxyManager();
  if (auto sname = pxm->GetProxyName("sources", input))
  {
    return sname;
  }
  else if (auto vname = pxm->GetProxyName("views", input))
  {
    return vname;
  }
  return "extract";
}

std::string DefaultFilename(vtkSMProxy* input, std::string pattern)
{
  const auto prefix = DefaultFilenamePrefix(input);
  vtksys::SystemTools::ReplaceString(pattern, "%p", prefix);
  return pattern;
}
}

vtkStandardNewMacro(vtkSMExtractsController);
//----------------------------------------------------------------------------
vtkSMExtractsController::vtkSMExtractsController()
  : TimeStep(0)
  , Time(0.0)
  , ExtractsOutputDirectory(nullptr)
  , EnvironmentExtractsOutputDirectory(nullptr)
  , SummaryTable(nullptr)
  , LastExtractsOutputDirectory{}
  , ExtractsOutputDirectoryValid(false)
{
  if (vtksys::SystemTools::HasEnv("PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY"))
  {
    this->SetEnvironmentExtractsOutputDirectory(
      vtksys::SystemTools::GetEnv("PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY"));
  }
}

//----------------------------------------------------------------------------
vtkSMExtractsController::~vtkSMExtractsController()
{
  this->SetExtractsOutputDirectory(nullptr);
  this->SetEnvironmentExtractsOutputDirectory(nullptr);
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::Extract()
{
  if (auto session = vtkSMSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetSession()))
  {
    return this->Extract(session->GetSessionProxyManager());
  }
  vtkErrorMacro("No active session!");
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::Extract(vtkSMSessionProxyManager* pxm)
{
  bool status = false;
  vtkNew<vtkSMProxyIterator> piter;
  piter->SetSessionProxyManager(pxm);
  for (piter->Begin("extractors"); !piter->IsAtEnd(); piter->Next())
  {
    status = this->Extract(piter->GetProxy()) || status;
  }
  return status;
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::Extract(vtkCollection* collection)
{
  bool status = false;
  auto range = vtk::Range(collection);
  for (auto item : range)
  {
    if (auto extractor = vtkSMProxy::SafeDownCast(item))
    {
      status = this->Extract(extractor) || status;
    }
  }
  return status;
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::Extract(vtkSMProxy* extractor)
{
  if (!this->IsTriggerActivated(extractor))
  {
    // skipping, nothing to do.
    return false;
  }

  if (!this->CreateExtractsOutputDirectory(extractor->GetSessionProxyManager()))
  {
    return false;
  }

  // Update filenames etc.
  if (auto writer = vtkSMExtractWriterProxy::SafeDownCast(
        vtkSMPropertyHelper(extractor, "Writer").GetAsProxy(0)))
  {
    if (!writer->Write(this))
    {
      vtkErrorMacro("Write failed! Extracts may not be generated correctly!");
      return false;
    }
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::IsAnyTriggerActivated(vtkSMSessionProxyManager* pxm)
{
  vtkNew<vtkSMProxyIterator> piter;
  piter->SetSessionProxyManager(pxm);
  for (piter->Begin("extractors"); !piter->IsAtEnd(); piter->Next())
  {
    if (this->IsTriggerActivated(piter->GetProxy()))
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::IsAnyTriggerActivated()
{
  if (auto session = vtkSMSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetSession()))
  {
    return this->IsAnyTriggerActivated(session->GetSessionProxyManager());
  }
  vtkErrorMacro("No active session!");
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::IsAnyTriggerActivated(vtkCollection* collection)
{
  auto range = vtk::Range(collection);
  for (auto item : range)
  {
    if (auto extractor = vtkSMProxy::SafeDownCast(item))
    {
      if (this->IsTriggerActivated(extractor))
      {
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::IsTriggerActivated(vtkSMProxy* extractor)
{
  if (!extractor)
  {
    vtkErrorMacro("Invalid 'extractor'.");
    return false;
  }

  if (!vtkSMExtractsController::IsExtractorEnabled(extractor))
  {
    // skipping, not enabled.
    return false;
  }

  auto trigger =
    vtkSMExtractTriggerProxy::SafeDownCast(vtkSMPropertyHelper(extractor, "Trigger").GetAsProxy(0));
  // note, if no trigger is provided, we assume it is always enabled.
  if (trigger != nullptr && !trigger->IsActivated(this))
  {
    // skipping, nothing to do.
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkSMProxy*> vtkSMExtractsController::FindExtractors(vtkSMProxy* proxy) const
{
  std::vector<vtkSMProxy*> result;
  if (!proxy)
  {
    return result;
  }

  auto pxm = proxy->GetSessionProxyManager();
  vtkNew<vtkSMProxyIterator> piter;
  piter->SetSessionProxyManager(pxm);
  for (piter->Begin("extractors"); !piter->IsAtEnd(); piter->Next())
  {
    auto extractor = piter->GetProxy();
    if (vtkSMExtractsController::IsExtractor(extractor, proxy))
    {
      result.push_back(extractor);
    }
  }
  return result;
}

//----------------------------------------------------------------------------
std::vector<vtkSMProxy*> vtkSMExtractsController::GetSupportedExtractorPrototypes(
  vtkSMProxy* proxy) const
{
  std::vector<vtkSMProxy*> result;
  if (!proxy)
  {
    return result;
  }

  auto pxm = proxy->GetSessionProxyManager();
  auto pdm = pxm->GetProxyDefinitionManager();
  auto piter = vtkSmartPointer<vtkPVProxyDefinitionIterator>::Take(
    pdm->NewSingleGroupIterator("extract_writers"));
  for (piter->InitTraversal(); !piter->IsDoneWithTraversal(); piter->GoToNextItem())
  {
    auto prototype = vtkSMExtractWriterProxy::SafeDownCast(
      pxm->GetPrototypeProxy("extract_writers", piter->GetProxyName()));
    if (prototype && prototype->CanExtract(proxy))
    {
      result.push_back(prototype);
    }
  }
  return result;
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::CanExtract(
  vtkSMProxy* extractor, const std::vector<vtkSMProxy*>& inputs) const
{
  if (!extractor || inputs.size() == 0)
  {
    return false;
  }

  // currently, we don't have any multiple input extractors.
  if (inputs.size() > 1)
  {
    return false;
  }

  // let's support the case where the 'extractor' is not really the extractor
  // but a writer prototype.
  auto writer = vtkSMExtractWriterProxy::SafeDownCast(extractor);
  if (!writer)
  {
    writer =
      vtkSMExtractWriterProxy::SafeDownCast(vtkSMPropertyHelper(extractor, "Writer").GetAsProxy());
  }

  return (writer && writer->CanExtract(inputs[0]));
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::IsExtractor(vtkSMProxy* extractor, vtkSMProxy* proxy)
{
  if (proxy == nullptr || extractor == nullptr)
  {
    return false;
  }

  auto writer =
    vtkSMExtractWriterProxy::SafeDownCast(vtkSMPropertyHelper(extractor, "Writer").GetAsProxy());
  return (writer && writer->IsExtracting(proxy)) ? true : false;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMExtractsController::GetInputForExtractor(vtkSMProxy* extractor) const
{
  if (extractor == nullptr)
  {
    return nullptr;
  }

  auto writer =
    vtkSMExtractWriterProxy::SafeDownCast(vtkSMPropertyHelper(extractor, "Writer").GetAsProxy());
  return (writer ? writer->GetInput() : nullptr);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMExtractsController::CreateExtractor(
  vtkSMProxy* proxy, const char* xmlname, const char* registrationName /*=nullptr*/) const
{
  if (!proxy)
  {
    return nullptr;
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  auto pxm = proxy->GetSessionProxyManager();
  auto writer = vtkSmartPointer<vtkSMExtractWriterProxy>::Take(
    vtkSMExtractWriterProxy::SafeDownCast(pxm->NewProxy("extract_writers", xmlname)));
  if (!writer)
  {
    return nullptr;
  }
  controller->PreInitializeProxy(writer);
  writer->SetInput(proxy);
  // don't call PostInitializeProxy here, let
  // vtkSMParaViewPipelineController::RegisterProxiesForProxyListDomains handle
  // that.
  // controller->PostInitializeProxy(writer);

  const std::string pname = registrationName
    ? std::string(registrationName)
    : pxm->GetUniqueProxyName("extractors", writer->GetXMLLabel());
  auto extractor = vtkSmartPointer<vtkSMProxy>::Take(pxm->NewProxy("extractors", "Extractor"));

  SM_SCOPED_TRACE(CreateExtractor)
    .arg("producer", proxy)
    .arg("extractor", extractor)
    .arg("xmlname", xmlname)
    .arg("registrationName", pname.c_str());

  controller->PreInitializeProxy(extractor);
  vtkSMPropertyHelper(extractor, "Writer").Set(writer);

  // this is done so that producer-consumer links are setup properly. Makes it easier
  // to delete the Extractor when the producer goes away.
  vtkSMProxy* producerProxy = nullptr;
  if (auto port = vtkSMOutputPort::SafeDownCast(proxy))
  {
    vtkSMPropertyHelper(extractor, "Producer").Set(port->GetSourceProxy());
    producerProxy = port->GetSourceProxy();
  }
  else
  {
    vtkSMPropertyHelper(extractor, "Producer").Set(proxy);
    producerProxy = proxy;
  }

  // update default filename.
  vtkSMPropertyHelper(writer, "FileName")
    .Set(::DefaultFilename(producerProxy, vtkSMPropertyHelper(writer, "FileName").GetAsString())
           .c_str());

  controller->PostInitializeProxy(extractor);
  extractor->UpdateVTKObjects();

  // add it to the proxy list domain too so the UI shows it correctly.
  // doing this before calling RegisterExtractorProxy also ensures that
  // it gets registered as a helper proxy.
  if (auto prop = extractor->GetProperty("Writer"))
  {
    if (auto pld = prop->FindDomain<vtkSMProxyListDomain>())
    {
      pld->AddProxy(writer);
    }
  }
  controller->RegisterExtractorProxy(extractor, pname.c_str());
  return extractor;
}

//----------------------------------------------------------------------------
const char* vtkSMExtractsController::GetRealExtractsOutputDirectory() const
{
  return (this->EnvironmentExtractsOutputDirectory ? this->EnvironmentExtractsOutputDirectory
                                                   : this->ExtractsOutputDirectory);
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::CreateExtractsOutputDirectory(vtkSMSessionProxyManager* pxm) const
{
  const auto output_directory = this->GetRealExtractsOutputDirectory();
  if (output_directory == nullptr)
  {
    vtkErrorMacro("ExtractsOutputDirectory must be specified.");
    return false;
  }

  if (this->LastExtractsOutputDirectory != output_directory)
  {
    this->ExtractsOutputDirectoryValid = this->CreateDirectory(output_directory, pxm);
    this->LastExtractsOutputDirectory = output_directory;
  }

  return this->ExtractsOutputDirectoryValid;
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::CreateDirectory(
  const std::string& dname, vtkSMSessionProxyManager* pxm) const
{
  if (dname.empty())
  {
    return false;
  }

  vtkNew<vtkSMFileUtilities> utils;
  utils->SetSession(pxm->GetSession());
  if (utils->MakeDirectory(dname, vtkPVSession::DATA_SERVER))
  {
    return true;
  }
  else
  {
    vtkErrorMacro("Failed to create directory: " << dname.c_str());
    return false;
  }
}

//----------------------------------------------------------------------------
void vtkSMExtractsController::ResetSummaryTable()
{
  this->SummaryTable = nullptr;
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::AddSummaryEntry(
  vtkSMExtractWriterProxy* writer, const std::string& filename, const SummaryParametersT& inparams)
{
  SummaryParametersT params(inparams);

  // add time, if not already present.
  params.insert({ "time", ::ConvertToString(this->Time) });
  // removing timestep from Cinema. I'm told only time (or timestep) should be
  // written; opting to use time.
  // params.insert({ "timestep", std::to_string(this->TimeStep) });
  params.insert({ vtkSMExtractsController::GetSummaryTableFilenameColumnName(filename),
    ::RelativePath(this->GetRealExtractsOutputDirectory(), filename) });

  // get a helpful name for this writer.
  auto name = this->GetName(writer);
  if (!name.empty())
  {
    params.insert({ "producer", name });
  }

  if (this->SummaryTable == nullptr)
  {
    this->SummaryTable.TakeReference(vtkTable::New());
    this->SummaryTable->Initialize();
  }

  auto table = this->SummaryTable;
  auto idx = table->GetNumberOfRows();
  for (const auto& pair : params)
  {
    if (auto array = vtkStringArray::SafeDownCast(table->GetColumnByName(pair.first.c_str())))
    {
      array->InsertValue(idx, pair.second);
    }
    else
    {
      vtkNew<vtkStringArray> narray;
      narray->SetName(pair.first.c_str());
      narray->InsertValue(idx, pair.second);
      table->GetRowData()->AddArray(narray);
    }
  }

  // resize all columns if needed.
  for (vtkIdType cc = 0, max = table->GetNumberOfColumns(); cc < max; ++cc)
  {
    auto column = vtkStringArray::SafeDownCast(table->GetColumn(cc));
    if (column && idx >= column->GetNumberOfTuples())
    {
      column->InsertValue(idx, std::string());
    }
  }
  return true;
}

//----------------------------------------------------------------------------
std::string vtkSMExtractsController::GetName(vtkSMExtractWriterProxy* writer)
{
  for (unsigned int cc = 0, max = writer->GetNumberOfConsumers(); cc < max; ++cc)
  {
    auto producer = writer->GetConsumerProxy(cc);
    auto pproperty = writer->GetConsumerProperty(cc);
    if (pproperty && strcmp(pproperty->GetXMLName(), "Writer") == 0 && producer &&
      strcmp(producer->GetXMLGroup(), "extractors") == 0 &&
      strcmp(producer->GetXMLName(), "Extractor") == 0)
    {
      auto pxm = producer->GetSessionProxyManager();
      auto name = pxm->GetProxyName("extractors", producer);
      return name;
    }
  }

  return writer->GetGlobalIDAsString();
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::SaveSummaryTable(
  const std::string& vtkNotUsed(fname), vtkSMSessionProxyManager* pxm)
{
  if (!this->SummaryTable || !pxm)
  {
    return false;
  }

  if (!this->CreateExtractsOutputDirectory(pxm))
  {
    return false;
  }

  auto writer = vtkSmartPointer<vtkSMProxy>::Take(pxm->NewProxy("misc_internals", "CDBWriter"));
  if (!writer)
  {
    vtkErrorMacro("Failed to create 'CDBWriter'. Cannot write summary table.");
    return false;
  }

  auto helper = vtkSmartPointer<vtkSMSourceProxy>::Take(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("misc", "RemoteWriterHelper")));
  if (!helper)
  {
    vtkErrorMacro("Failed to create 'RemoteWriterHelper' proxy. Cannot write summary table.");
    return false;
  }

  writer->SetLocation(helper->GetLocation());
  vtkSMPropertyHelper(writer, "FileName").Set(this->GetRealExtractsOutputDirectory());
  writer->UpdateVTKObjects();
  vtkSMPropertyHelper(helper, "OutputDestination").Set(vtkPVSession::DATA_SERVER_ROOT);
  vtkSMPropertyHelper(helper, "Writer").Set(writer);
  helper->UpdateVTKObjects();

  vtkAlgorithm::SafeDownCast(helper->GetClientSideObject())->SetInputDataObject(this->SummaryTable);
  helper->UpdatePipeline();
  return true;
}

//----------------------------------------------------------------------------
std::string vtkSMExtractsController::GetSummaryTableFilenameColumnName(const std::string& fname)
{
  auto ext = vtksys::SystemTools::GetFilenameLastExtension(fname);
  if (!ext.empty())
  {
    // remote the dot.
    ext.erase(ext.begin());
    ext = vtksys::SystemTools::LowerCase(ext);
  }
  return ext.empty() ? "FILE" : ("FILE_" + ext);
}

//----------------------------------------------------------------------------
vtkTable* vtkSMExtractsController::GetSummaryTable() const
{
  return this->SummaryTable.GetPointer();
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::IsExtractorEnabled(vtkSMProxy* extractor)
{
  return (extractor && vtkSMPropertyHelper(extractor, "Enable").GetAsInt() != 0);
}

//----------------------------------------------------------------------------
void vtkSMExtractsController::SetExtractorEnabled(vtkSMProxy* extractor, bool val)
{
  if (extractor)
  {
    vtkSMPropertyHelper(extractor, "Enable").Set(val ? 1 : 0);
    extractor->UpdateVTKObjects();
  }
}

//----------------------------------------------------------------------------
void vtkSMExtractsController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "Time: " << this->Time << endl;
  os << indent << "ExtractsOutputDirectory: "
     << (this->ExtractsOutputDirectory ? this->ExtractsOutputDirectory : "(nullptr)") << endl;
}
