/*=========================================================================

  Program:   ParaView
  Module:    ParaViewFidesEngine.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef PARAVIEWFIDESENGINE_H
#define PARAVIEWFIDESENGINE_H

#include "paraviewadiosinsituengine_export.h"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/IO.h"
#include "adios2/engine/plugin/PluginEngineInterface.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosType.h"

#include <memory>

namespace fides_plugin
{

/**
 *  ParaViewFidesEngine: An engine plugin for ADIOS2 that supports in situ
 *  visualization with the Inline engine. This engine handles the writing
 *  side of things by forwarding Puts onto the Inline writer.
 *  To handle to read side of things, a ParaView Fides reader is set up
 *  so the written data can be used in a visualization pipeline.
 *  Finally, the engine uses ParaView's catalyst infrastructure to run
 *  python scripts that set up the visualization pipeline.
 *
 *  Parameters to be passed to ADIOS to use this engine:
 *  Key -> Value
 *  "Script" -> "/path/to/python/script"
 *      - Python script that sets up ParaView pipeline for visualization
 *  "DataModel" -> "/path/to/json"
 *      - Fides JSON data model file
 *
 */
class ParaViewFidesEngine : public adios2::plugin::PluginEngineInterface
{

public:
  ParaViewFidesEngine(adios2::core::IO& adios, const std::string& name, adios2::helper::Comm comm);

  ~ParaViewFidesEngine() override;

  adios2::StepStatus BeginStep(adios2::StepMode mode, const float timeoutSeconds = -1.0) override;

  void EndStep() override;

  size_t CurrentStep() const override;

  void PerformPuts() override;

protected:
#define declare_type(T)                                                                            \
  void DoPutSync(adios2::core::Variable<T>&, const T*) override;                                   \
  void DoPutDeferred(adios2::core::Variable<T>&, const T*) override;
  ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

  void DoClose(const int transportIndex = -1) override;

private:
  struct EngineImpl;
  std::unique_ptr<EngineImpl> Impl;
};

} // end namespace fides_plugin

extern "C"
{

  PARAVIEWADIOSINSITUENGINE_EXPORT fides_plugin::ParaViewFidesEngine* EngineCreate(
    adios2::core::IO& io, const std::string& name, const adios2::Mode mode,
    adios2::helper::Comm comm);
  PARAVIEWADIOSINSITUENGINE_EXPORT void EngineDestroy(fides_plugin::ParaViewFidesEngine* obj);
}

#endif // PARAVIEWFIDESENGINE_H
