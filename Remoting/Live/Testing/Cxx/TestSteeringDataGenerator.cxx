/*=========================================================================

Program:   ParaView
Module:    TestSteeringDataGenerator.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAlgorithm.h"
#include "vtkIdTypeArray.h"
#include "vtkInitializationHelper.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkPVOptions.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkVector.h"

const char* test_xml = R"(
<ServerManagerConfiguration>
 <ProxyGroup name="sources">
    <!-- ==================================================================== -->
    <SourceProxy class="vtkSteeringDataGenerator" name="TestSteeringDataGeneratorSource">

      <IntVectorProperty name="PartitionType"
                         command="SetPartitionType"
                         number_of_elements="1"
                         default_values="0"
                         panel_visibility="never">
        <!-- 0 == VTK_POLY_DATA -->
      </IntVectorProperty>

      <IntVectorProperty name="FieldAssociation"
                         command="SetFieldAssociation"
                         number_of_elements="1"
                         default_values="0"
                         panel_visibility="never" />

      <DoubleVectorProperty name="Center"
                            command="SetTuple3Double"
                            use_index="1"
                            clean_command="Clear"
                            initial_string="coords"
                            number_of_elements_per_command="3"
                            repeat_command="1" />

      <IdTypeVectorProperty name="Id"
                            command="SetTuple2IdType"
                            clean_command="Clear"
                            use_index="1"
                            initial_string="Id"
                            number_of_elements_per_command="2"
                            repeat_command="1" />

      <IntVectorProperty name="Type"
                         command="SetTuple1Int"
                         clean_command="Clear"
                         use_index="1"
                         initial_string="Type"
                         number_of_elements_per_command="1"
                         repeat_command="1" />
    </SourceProxy>
  </ProxyGroup>
</ServerManagerConfiguration>
)";

//----------------------------------------------------------------------------
int TestSteeringDataGenerator(int argc, char* argv[])
{
  vtkNew<vtkPVOptions> options;
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_CLIENT, options);

  auto session = vtkSMSession::New();
  auto pxm = session->GetSessionProxyManager();
  pxm->GetProxyDefinitionManager()->LoadConfigurationXMLFromString(test_xml);

  auto proxy =
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "TestSteeringDataGeneratorSource"));
  assert(proxy != nullptr);

  const vtkVector3d centers[2] = { { 0, 0, 0 }, { 1, 1, 1 } };
  vtkSMPropertyHelper(proxy, "Center").Set(&centers[0][0], 6);

  const vtkVector2<vtkIdType> ids[2] = { { 100, 101 }, { 200, 201 } };
  vtkSMPropertyHelper(proxy, "Id").Set(&ids[0][0], 4);

  const int types[2] = { 1000, 1001 };
  vtkSMPropertyHelper(proxy, "Type").Set(types, 2);
  proxy->UpdateVTKObjects();
  proxy->UpdatePipeline();

  // validate dataset.
  auto algo = vtkAlgorithm::SafeDownCast(proxy->GetClientSideObject());
  auto mb = vtkMultiBlockDataSet::SafeDownCast(algo->GetOutputDataObject(0));
  if (mb->GetNumberOfBlocks() != 1)
  {
    vtkLogF(ERROR, "Partition mismatch; expected 1, got %d", (int)mb->GetNumberOfBlocks());
    return EXIT_FAILURE;
  }

  auto pld = vtkPolyData::SafeDownCast(mb->GetBlock(0));
  if (pld == nullptr)
  {
    vtkLogF(ERROR, "Partition type mismatch; expected vtkPolyData.");
    return EXIT_FAILURE;
  }

  if (pld->GetNumberOfPoints() != 2 || vtkVector3d(pld->GetPoint(0)) != centers[0] ||
    vtkVector3d(pld->GetPoint(1)) != centers[1])
  {
    vtkLogF(ERROR, "Points mismatch; expected and got point values differ!");
    return EXIT_FAILURE;
  }

  auto pointData = pld->GetPointData();
  auto typeA = vtkIntArray::SafeDownCast(pointData->GetArray("Type"));
  if (typeA == nullptr || typeA->GetNumberOfComponents() != 1 || typeA->GetNumberOfTuples() != 2 ||
    typeA->GetTypedComponent(0, 0) != types[0] || typeA->GetTypedComponent(1, 0) != types[1])
  {
    vtkLogF(ERROR, "'Type' array mismatch!");
    return EXIT_FAILURE;
  }

  auto idsA = vtkIdTypeArray::SafeDownCast(pointData->GetArray("Id"));
  if (idsA == nullptr || idsA->GetNumberOfComponents() != 2 || idsA->GetNumberOfTuples() != 2 ||
    vtkVector2<vtkIdType>(idsA->GetPointer(0)) != ids[0] ||
    vtkVector2<vtkIdType>(idsA->GetPointer(2)) != ids[1])
  {
    vtkLogF(ERROR, "'Id' array mismatch!");
    return EXIT_FAILURE;
  }

  proxy->Delete();
  session->Delete();
  vtkInitializationHelper::Finalize();
  return EXIT_SUCCESS;
}
