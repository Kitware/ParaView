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
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkProcessModule.h"
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

#if VTK_MODULE_ENABLE_VTK_IOCore
#include "vtkDelimitedTextWriter.h"
#endif

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
}

vtkStandardNewMacro(vtkSMExtractsController);
//----------------------------------------------------------------------------
vtkSMExtractsController::vtkSMExtractsController()
  : TimeStep(0)
  , Time(0.0)
  , ExtractsOutputDirectory(nullptr)
  , SummaryTable(nullptr)
  , LastExtractsOutputDirectory{}
  , ExtractsOutputDirectoryValid(false)
{
}

//----------------------------------------------------------------------------
vtkSMExtractsController::~vtkSMExtractsController()
{
  this->SetExtractsOutputDirectory(nullptr);
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
  for (piter->Begin("extract_generators"); !piter->IsAtEnd(); piter->Next())
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
    if (auto generator = vtkSMProxy::SafeDownCast(item))
    {
      status = this->Extract(generator) || status;
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
  for (piter->Begin("extract_generators"); !piter->IsAtEnd(); piter->Next())
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
    if (auto generator = vtkSMProxy::SafeDownCast(item))
    {
      if (this->IsTriggerActivated(generator))
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

  if (vtkSMPropertyHelper(extractor, "EnableExtract").GetAsInt() != 1)
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
std::vector<vtkSMProxy*> vtkSMExtractsController::FindExtractGenerators(vtkSMProxy* proxy) const
{
  std::vector<vtkSMProxy*> result;
  if (!proxy)
  {
    return result;
  }

  auto pxm = proxy->GetSessionProxyManager();
  vtkNew<vtkSMProxyIterator> piter;
  piter->SetSessionProxyManager(pxm);
  for (piter->Begin("extract_generators"); !piter->IsAtEnd(); piter->Next())
  {
    auto generator = piter->GetProxy();
    if (vtkSMExtractsController::IsExtractGenerator(generator, proxy))
    {
      result.push_back(generator);
    }
  }
  return result;
}

//----------------------------------------------------------------------------
std::vector<vtkSMProxy*> vtkSMExtractsController::GetSupportedExtractGeneratorPrototypes(
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
  vtkSMProxy* generator, const std::vector<vtkSMProxy*>& inputs) const
{
  if (!generator || inputs.size() == 0)
  {
    return false;
  }

  // currently, we don't have any multiple input extract generators.
  if (inputs.size() > 1)
  {
    return false;
  }

  // let's support the case where the 'generator' is not really the generator
  // but a writer prototype.
  auto writer = vtkSMExtractWriterProxy::SafeDownCast(generator);
  if (!writer)
  {
    writer =
      vtkSMExtractWriterProxy::SafeDownCast(vtkSMPropertyHelper(generator, "Writer").GetAsProxy());
  }

  return (writer && writer->CanExtract(inputs[0]));
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::IsExtractGenerator(vtkSMProxy* generator, vtkSMProxy* proxy)
{
  if (proxy == nullptr || generator == nullptr)
  {
    return false;
  }

  auto writer =
    vtkSMExtractWriterProxy::SafeDownCast(vtkSMPropertyHelper(generator, "Writer").GetAsProxy());
  return (writer && writer->IsExtracting(proxy)) ? true : false;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMExtractsController::GetInputForGenerator(vtkSMProxy* generator) const
{
  if (generator == nullptr)
  {
    return nullptr;
  }

  auto writer =
    vtkSMExtractWriterProxy::SafeDownCast(vtkSMPropertyHelper(generator, "Writer").GetAsProxy());
  return (writer ? writer->GetInput() : nullptr);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMExtractsController::CreateExtractGenerator(
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

  const std::string pname = registrationName
    ? std::string(registrationName)
    : pxm->GetUniqueProxyName("extract_generators", writer->GetXMLLabel());
  auto generator =
    vtkSmartPointer<vtkSMProxy>::Take(pxm->NewProxy("extract_generators", "Extractor"));
  // add it to the proxy list domain too so the UI shows it correctly.
  if (auto prop = generator->GetProperty("Writer"))
  {
    if (auto pld = prop->FindDomain<vtkSMProxyListDomain>())
    {
      pld->AddProxy(writer);
    }
  }

  SM_SCOPED_TRACE(CreateExtractGenerator)
    .arg("producer", proxy)
    .arg("generator", generator)
    .arg("xmlname", xmlname)
    .arg("registrationName", pname.c_str());

  controller->PreInitializeProxy(generator);
  writer->SetInput(proxy);
  vtkSMPropertyHelper(generator, "Writer").Set(writer);

  // this is done so that producer-consumer links are setup properly. Makes it easier
  // to delete the Extractor when the producer goes away.
  if (auto port = vtkSMOutputPort::SafeDownCast(proxy))
  {
    vtkSMPropertyHelper(generator, "Producer").Set(port->GetSourceProxy());
  }
  else
  {
    vtkSMPropertyHelper(generator, "Producer").Set(proxy);
  }
  controller->PostInitializeProxy(generator);
  generator->UpdateVTKObjects();
  controller->RegisterExtractGeneratorProxy(generator, pname.c_str());

  return generator;
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::CreateExtractsOutputDirectory(vtkSMSessionProxyManager* pxm) const
{
  if (this->ExtractsOutputDirectory == nullptr)
  {
    vtkErrorMacro("ExtractsOutputDirectory must be specified.");
    return false;
  }

  if (this->LastExtractsOutputDirectory != this->ExtractsOutputDirectory)
  {
    this->ExtractsOutputDirectoryValid = this->CreateDirectory(this->ExtractsOutputDirectory, pxm);
    this->LastExtractsOutputDirectory = this->ExtractsOutputDirectory;
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
  params.insert({ "timestep", std::to_string(this->TimeStep) });
  params.insert({ vtkSMExtractsController::GetSummaryTableFilenameColumnName(filename),
    vtksys::SystemTools::RelativePath(this->ExtractsOutputDirectory, filename) });

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
      table->AddColumn(narray);
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
      strcmp(producer->GetXMLGroup(), "extract_generators") == 0 &&
      strcmp(producer->GetXMLName(), "Extractor") == 0)
    {
      auto pxm = producer->GetSessionProxyManager();
      auto name = pxm->GetProxyName("extract_generators", producer);
      return name;
    }
  }

  return writer->GetGlobalIDAsString();
}

//----------------------------------------------------------------------------
bool vtkSMExtractsController::SaveSummaryTable(
  const std::string& fname, vtkSMSessionProxyManager* pxm)
{
#if VTK_MODULE_ENABLE_VTK_IOCore
  if (!this->SummaryTable || !pxm)
  {
    return false;
  }

  if (!this->CreateExtractsOutputDirectory(pxm))
  {
    return false;
  }

  // TODO: in client-server mode, the file must be saved on the server-side.
  vtkNew<vtkDelimitedTextWriter> writer;
  writer->SetInputDataObject(this->SummaryTable);
  writer->SetFileName(
    vtksys::SystemTools::JoinPath({ this->ExtractsOutputDirectory, "/" + fname }).c_str());
  return writer->Write() != 0;
#else
  vtkErrorMacro("VTK::IOCore module not built. Cannot save summary table.");
  return false;
#endif
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
void vtkSMExtractsController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "Time: " << this->Time << endl;
  os << indent << "ExtractsOutputDirectory: "
     << (this->ExtractsOutputDirectory ? this->ExtractsOutputDirectory : "(nullptr)") << endl;
}
