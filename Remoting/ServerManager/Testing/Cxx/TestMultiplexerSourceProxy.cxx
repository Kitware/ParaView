/*=========================================================================

Program:   ParaView
Module:    TestMultiplexerSourceProxy.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkProcessModule.h"
#include "vtkSMMultiplexerSourceProxy.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkWeakPointer.h"

/* clang-format off */
static const char* testMultiplexerSourceProxyXML = R"==(
<ServerManagerConfiguration>
  <ProxyGroup name="filters">
    <MultiplexerSourceProxy name="Multiplexer">
      <InputProperty name="Input">
        <Documentation>
          Specify the input to this filter.
        </Documentation>
        <MultiplexerInputDomain name="input" />
      </InputProperty>
    </MultiplexerSourceProxy>

    <SourceProxy name="TypeSphere" class="vtkSphereSource">
      <InputProperty name="Input">
        <DataTypeDomain name="input_type">
          <DataType value="vtkPolyData" />
        </DataTypeDomain>
      </InputProperty>
      <DoubleVectorProperty animateable="1"
                            command="SetCenter"
                            default_values="0.0 0.0 0.0"
                            name="Center"
                            number_of_elements="3"
                            panel_visibility="default">
        <DoubleRangeDomain name="range" />
        <Documentation>This property specifies the 3D coordinates for the
        center of the sphere.</Documentation>
      </DoubleVectorProperty>

      <Hints>
        <!-- not adding optional <LinkProperties/> element -->
        <MultiplexerSourceProxy proxygroup="filters" proxyname="Multiplexer" />
      </Hints>
    </SourceProxy>

    <SourceProxy name="TypeWavelet" class="vtkRTAnalyticSource">
      <InputProperty name="Input0">
        <DataTypeDomain name="input_type">
          <DataType value="vtkImageData" />
        </DataTypeDomain>
      </InputProperty>
      <IntVectorProperty command="SetWholeExtent"
                         default_values="-10 10 -10 10 -10 10"
                         label="Whole Extent"
                         name="WholeExtent"
                         number_of_elements="6"
                         panel_visibility="default">
        <IntRangeDomain name="range" />
      </IntVectorProperty>
      <DoubleVectorProperty animateable="1"
                            command="SetXFreq"
                            default_values="60.0"
                            name="XFreq"
                            number_of_elements="1"
                            panel_visibility="advanced">
        <DoubleRangeDomain name="range" />
        <Documentation>This property specifies the natural frequency in X (XF
        in the equation).</Documentation>
      </DoubleVectorProperty>
      <DoubleVectorProperty animateable="1"
                            command="SetYFreq"
                            default_values="30.0"
                            name="YFreq"
                            number_of_elements="1"
                            panel_visibility="advanced">
        <DoubleRangeDomain name="range" />
        <Documentation>This property specifies the natural frequency in Y (YF
        in the equation).</Documentation>
      </DoubleVectorProperty>
      <DoubleVectorProperty animateable="1"
                            command="SetZFreq"
                            default_values="40.0"
                            name="ZFreq"
                            number_of_elements="1"
                            panel_visibility="advanced">
        <DoubleRangeDomain name="range" />
        <Documentation>This property specifies the natural frequency in Z (ZF
        in the equation).</Documentation>
      </DoubleVectorProperty>

      <PropertyGroup label="Frequency">
        <Property name="XFreq" />
        <Property name="YFreq" />
        <Property name="ZFreq" />
      </PropertyGroup>

      <Hints>
        <MultiplexerSourceProxy proxygroup="filters" proxyname="Multiplexer">
          <LinkProperties>
            <!-- adding custom property linking -->
            <Property name="Input0" with_property="Input" />
          </LinkProperties>
        </MultiplexerSourceProxy>
      </Hints>
    </SourceProxy>
  </ProxyGroup>
</ServerManagerConfiguration>
)==";

  /* clang-format on */

  static bool
  ValidateSphere(vtkSMProxy* mux)
{
  if (mux->GetProperty("Center") != nullptr)
  {
    vtkSMPropertyHelper(mux, "Center").Set(0, 12);
    mux->UpdateVTKObjects();
  }
  else
  {
    vtkLogF(ERROR, "Missing 'Center' property!");
    return false;
  }

  if (mux->GetProperty("ZFreq") != nullptr)
  {
    vtkLogF(ERROR, "Unexpected property 'ZFreq!");
    return false;
  }
  return true;
}

static bool ValidateWavelet(vtkSMProxy* mux)
{
  if (mux->GetProperty("ZFreq") != nullptr)
  {
    vtkSMPropertyHelper(mux, "ZFreq").Set(12);
    mux->UpdateVTKObjects();
  }
  else
  {
    vtkLogF(ERROR, "Missing 'ZFreq' property!");
    return false;
  }

  if (mux->GetProperty("Center") != nullptr)
  {
    vtkLogF(ERROR, "Unexpected property 'Center'!");
    return false;
  }

  if (mux->GetNumberOfPropertyGroups() != 1 || mux->GetPropertyGroup(0) == nullptr)
  {
    vtkLogF(ERROR, "Missing property group!");
    return false;
  }
  return true;
}

int TestMultiplexerSourceProxy(int argc, char* argv[])
{
  vtkNew<vtkPVTestUtilities> testing;
  testing->Initialize(argc, argv);

  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  vtkNew<vtkSMParaViewPipelineController> controller;

  // Create a new session.
  vtkNew<vtkSMSession> session;
  controller->InitializeSession(session);

  auto pxm = session->GetSessionProxyManager();
  pxm->LoadConfigurationXML(testMultiplexerSourceProxyXML);

  auto pdsource = vtkSmartPointer<vtkSMProxy>::Take(pxm->NewProxy("sources", "ConeSource"));
  controller->InitializeProxy(pdsource);
  controller->RegisterPipelineProxy(pdsource);

  auto mux = vtkSmartPointer<vtkSMSourceProxy>::Take(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "Multiplexer")));
  controller->PreInitializeProxy(mux);
  vtkSMPropertyHelper(mux, "Input").Set(pdsource);
  controller->PostInitializeProxy(mux);
  controller->RegisterPipelineProxy(mux, "mux0");

  if (!ValidateSphere(mux))
  {
    vtkInitializationHelper::Finalize();
    return EXIT_FAILURE;
  }

  // now try the img input.
  auto imgsource = vtkSmartPointer<vtkSMProxy>::Take(pxm->NewProxy("sources", "RTAnalyticSource"));
  controller->InitializeProxy(imgsource);
  controller->RegisterPipelineProxy(imgsource);

  mux = vtkSmartPointer<vtkSMSourceProxy>::Take(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "Multiplexer")));
  controller->PreInitializeProxy(mux);
  vtkSMPropertyHelper(mux, "Input").Set(imgsource);
  controller->PostInitializeProxy(mux);
  controller->RegisterPipelineProxy(mux, "mux1");

  if (!ValidateWavelet(mux))
  {
    vtkInitializationHelper::Finalize();
    return EXIT_FAILURE;
  }

  auto cstr = testing->GetTempFilePath("TestMultiplexerSourceProxy-state.pvsm");
  const std::string filename(cstr);
  delete[] cstr;
  pxm->SaveXMLState(filename.c_str());
  pxm->UnRegisterProxies();
  pdsource = nullptr;
  imgsource = nullptr;
  mux = nullptr;

  pxm->LoadXMLState(filename.c_str());
  auto mux0 = pxm->GetProxy("sources", "mux0");
  if (!mux0 || !ValidateSphere(mux0))
  {
    vtkLogF(ERROR, "State loading failed!");
    vtkInitializationHelper::Finalize();
    return EXIT_FAILURE;
  }
  auto mux1 = pxm->GetProxy("sources", "mux1");
  if (!mux1 || !ValidateWavelet(mux1))
  {
    vtkLogF(ERROR, "State loading failed!");
    vtkInitializationHelper::Finalize();
    return EXIT_FAILURE;
  }

  vtkInitializationHelper::Finalize();
  return EXIT_SUCCESS;
}
