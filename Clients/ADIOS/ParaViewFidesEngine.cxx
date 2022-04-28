/*=========================================================================

  Program:   ParaView
  Module:    ParaViewFidesEngine.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "ParaViewFidesEngine.h"

#include <adios2/engine/inline/InlineWriter.h>

#include <catalyst.hpp>

#include <iostream>
#include <sstream>

namespace fides_plugin
{

struct ParaViewFidesEngine::EngineImpl
{
  adios2::core::IO* Io;
  adios2::core::Engine* Writer;

  std::string ScriptFileName;
  std::string JSONFileName;

  int Rank;

  EngineImpl(adios2::core::ADIOS& adios)
  {
    this->Io = &adios.DeclareIO("InlinePluginIO");
    this->Io->SetEngine("inline");
    this->Writer = &Io->Open("write", adios2::Mode::Write);
  }

  void CatalystConfig()
  {
    std::cout << "\tCatalyst Library Version: " << CATALYST_VERSION << "\n";
    std::cout << "\tCatalyst ABI Version: " << CATALYST_ABI_VERSION << "\n";

    conduit_cpp::Node node;
    catalyst_about(conduit_cpp::c_node(&node));
    auto implementation = node.has_path("catalyst/implementation")
      ? node["catalyst/implementation"].as_string()
      : std::string("stub");
    std::cout << "\tImplementation: " << implementation << "\n\n";
  }

  void CatalystInit()
  {
    conduit_cpp::Node node;
    node["catalyst/scripts/script/filename"].set(this->ScriptFileName);

    // options to set up the fides reader in paraview
    std::ostringstream address;
    address << &Io;

    node["catalyst/fides/json_file"].set(this->JSONFileName);
    node["catalyst/fides/data_source_io/source"].set(std::string("source"));
    node["catalyst/fides/data_source_io/address"].set(address.str());
    node["catalyst/fides/data_source_path/source"].set(std::string("source"));
    node["catalyst/fides/data_source_path/path"].set(std::string("DataReader"));
    catalyst_initialize(conduit_cpp::c_node(&node));

    if (this->Rank == 0)
    {
      this->CatalystConfig();
    }
  }

  void CatalystExecute()
  {
    auto timestep = this->Writer->CurrentStep();
    conduit_cpp::Node node;
    node["catalyst/state/timestep"].set(timestep);
    // catalyst requires the next one, but when using Fides as the reader for
    // Catalyst, it will grab the time from the correct adios variable if it is
    // specified in the data model
    node["catalyst/state/time"].set(timestep);
    node["catalyst/channels/fides/type"].set(std::string("fides"));

    // catalyst requires the data node on a channel, but we don't actually
    // need it when using fides, so just create a dummy object to pass
    // the validation in catalyst
    conduit_cpp::Node dummy;
    dummy["dummy"].set(0);
    node["catalyst/channels/fides/data"].set(dummy);

    catalyst_execute(conduit_cpp::c_node(&node));
  }
};

ParaViewFidesEngine::ParaViewFidesEngine(
  adios2::core::IO& io, const std::string& name, adios2::helper::Comm comm)
  : adios2::plugin::PluginEngineInterface(io, name, adios2::Mode::Write, comm.Duplicate())
  , Impl(new EngineImpl(io.m_ADIOS))
{
  // Need to define the Variables in the IO object used for the inline engine
  const auto& varMap = io.GetVariables();
  for (const auto& it : varMap)
  {
    if (it.second->m_Type == adios2::DataType::Compound)
    {
    }
#define declare_type(T)                                                                            \
  else if (it.second->m_Type == adios2::helper::GetDataType<T>())                                  \
  {                                                                                                \
    this->Impl->Io->DefineVariable<T>(it.first, it.second->m_Shape, it.second->m_Start,            \
      it.second->m_Count, it.second->IsConstantDims());                                            \
  }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
  }

  this->Impl->Rank = comm.Rank();

  const auto& scriptIt = this->m_IO.m_Parameters.find("Script");
  if (scriptIt != this->m_IO.m_Parameters.end())
  {
    this->Impl->ScriptFileName = scriptIt->second;
  }

  // TODO required for now, but support data model generation in the future
  const auto& fileIt = this->m_IO.m_Parameters.find("DataModel");
  if (fileIt == this->m_IO.m_Parameters.end())
  {
    throw std::runtime_error("couldn't find DataModel in parameters!");
  }
  this->Impl->JSONFileName = fileIt->second;

  this->Impl->CatalystInit();
}

ParaViewFidesEngine::~ParaViewFidesEngine()
{
  conduit_cpp::Node node;
  catalyst_finalize(conduit_cpp::c_node(&node));
}

adios2::StepStatus ParaViewFidesEngine::BeginStep(adios2::StepMode mode, const float timeoutSeconds)
{
  return this->Impl->Writer->BeginStep(mode, timeoutSeconds);
}

size_t ParaViewFidesEngine::CurrentStep() const
{
  return this->Impl->Writer->CurrentStep();
}

void ParaViewFidesEngine::EndStep()
{
  this->Impl->Writer->EndStep();

  this->Impl->CatalystExecute();
}

void ParaViewFidesEngine::PerformPuts()
{
  this->Impl->Writer->PerformPuts();
}

#define declare(T)                                                                                 \
  void ParaViewFidesEngine::DoPutSync(adios2::core::Variable<T>& variable, const T* values)        \
  {                                                                                                \
    adios2::core::Variable<T>* inlineVar = this->Impl->Io->InquireVariable<T>(variable.m_Name);    \
    this->Impl->Writer->Put(*inlineVar, values, adios2::Mode::Sync);                               \
  }                                                                                                \
  void ParaViewFidesEngine::DoPutDeferred(adios2::core::Variable<T>& variable, const T* values)    \
  {                                                                                                \
    adios2::core::Variable<T>* inlineVar = this->Impl->Io->InquireVariable<T>(variable.m_Name);    \
    this->Impl->Writer->Put(*inlineVar, values);                                                   \
  }
ADIOS2_FOREACH_STDTYPE_1ARG(declare)
#undef declare

void ParaViewFidesEngine::DoClose(const int transportIndex)
{
  this->Impl->Writer->Close(transportIndex);
}

} // end namespace fides_plugin

extern "C"
{

  fides_plugin::ParaViewFidesEngine* EngineCreate(adios2::core::IO& io, const std::string& name,
    const adios2::Mode mode, adios2::helper::Comm comm)
  {
    (void)mode;
    return new fides_plugin::ParaViewFidesEngine(io, name, comm.Duplicate());
  }

  void EngineDestroy(fides_plugin::ParaViewFidesEngine* obj) { delete obj; }
}
