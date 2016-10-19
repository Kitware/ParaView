/*=========================================================================

  Program:   ParaView
  Module:    ServersServerManagerPrintSelf.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVConfig.h"

//#include "vtkInitializationHelper.h"
#include "vtkPVComparativeAnimationCue.h"
#include "vtkPVComparativeView.h"
#include "vtkPVKeyFrameAnimationCueForProxies.h"
#include "vtkPVRepresentationAnimationHelper.h"
#include "vtkSMAnimationScene.h"
#include "vtkSMAnimationSceneGeometryWriter.h"
#include "vtkSMAnimationSceneImageWriter.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMAnimationSceneWriter.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMArrayRangeDomain.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMBooleanDomain.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMBoxRepresentationProxy.h"
#include "vtkSMCSVExporterProxy.h"
#include "vtkSMCameraConfigurationReader.h"
#include "vtkSMCameraConfigurationWriter.h"
#include "vtkSMCameraLink.h"
#include "vtkSMCameraProxy.h"
#include "vtkSMChartRepresentationProxy.h"
#include "vtkSMComparativeAnimationCueProxy.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMCompoundProxyDefinitionLoader.h"
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMCompoundSourceProxyDefinitionBuilder.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMDeserializer.h"
#include "vtkSMDimensionsDomain.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMExporterProxy.h"
#include "vtkSMExtentDomain.h"
#include "vtkSMFieldDataDomain.h"
#include "vtkSMFileListDomain.h"
#include "vtkSMFixedTypeDomain.h"
#include "vtkSMGlobalPropertiesLinkUndoElement.h"
#include "vtkSMGlobalPropertiesProxy.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMImplicitPlaneRepresentationProxy.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMInteractionUndoStackBuilder.h"
#include "vtkSMLink.h"
#include "vtkSMNamedPropertyIterator.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMNullProxy.h"
#include "vtkSMNumberOfComponentsDomain.h"
#include "vtkSMObject.h"
#include "vtkSMOrderedPropertyIterator.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPSWriterProxy.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPWriterProxy.h"
#include "vtkSMPipelineState.h"
#include "vtkSMPluginLoaderProxy.h"
#include "vtkSMPluginManager.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMPropertyModificationUndoElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyConfigurationReader.h"
#include "vtkSMProxyConfigurationWriter.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMRemoteObject.h"
#include "vtkSMRemoteObjectUpdateUndoElement.h"
#include "vtkSMRenderViewExporterProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSILDomain.h"
#include "vtkSMSILModel.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionClient.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMSpreadSheetRepresentationProxy.h"
#include "vtkSMStateLoader.h"
#include "vtkSMStateLocator.h"
#include "vtkSMStateVersionController.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTesting.h"
#include "vtkSMTextWidgetRepresentationProxy.h"
#include "vtkSMTimeKeeper.h"
#include "vtkSMTimeKeeperProxy.h"
#include "vtkSMUndoElement.h"
#include "vtkSMUndoStack.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkSMUtilities.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMViewProxy.h"
#include "vtkSMWidgetRepresentationProxy.h"
#include "vtkSMWriterFactory.h"
#include "vtkSMWriterProxy.h"

#define PRINT_SELF(classname)                                                                      \
  cout << "------------------------------------" << endl;                                          \
  cout << "Class: " << #classname << endl;                                                         \
  c = classname::New();                                                                            \
  c->Print(cout);                                                                                  \
  c->Delete();

int ParaViewCoreServerManagerPrintSelf(int, char* [])
{
  vtkObject* c;

  //  PRINT_SELF(vtkInitializationHelper);
  PRINT_SELF(vtkPVComparativeAnimationCue);
  PRINT_SELF(vtkPVComparativeView);
  PRINT_SELF(vtkPVKeyFrameAnimationCueForProxies);
  PRINT_SELF(vtkPVRepresentationAnimationHelper);
  PRINT_SELF(vtkSMAnimationScene);
  PRINT_SELF(vtkSMAnimationSceneGeometryWriter);
  PRINT_SELF(vtkSMAnimationSceneImageWriter);
  PRINT_SELF(vtkSMAnimationSceneProxy);
  PRINT_SELF(vtkSMAnimationSceneWriter);
  PRINT_SELF(vtkSMArrayListDomain);
  PRINT_SELF(vtkSMArrayRangeDomain);
  PRINT_SELF(vtkSMArraySelectionDomain);
  PRINT_SELF(vtkSMBooleanDomain);
  PRINT_SELF(vtkSMBoundsDomain);
  PRINT_SELF(vtkSMBoxRepresentationProxy);
  PRINT_SELF(vtkSMCameraConfigurationReader);
  PRINT_SELF(vtkSMCameraConfigurationWriter);
  PRINT_SELF(vtkSMCameraLink);
  PRINT_SELF(vtkSMCameraProxy);
  PRINT_SELF(vtkSMChartRepresentationProxy);
  PRINT_SELF(vtkSMComparativeAnimationCueProxy);
  PRINT_SELF(vtkSMComparativeViewProxy);
  PRINT_SELF(vtkSMCompositeTreeDomain);
  PRINT_SELF(vtkSMCompoundProxyDefinitionLoader);
  PRINT_SELF(vtkSMCompoundSourceProxy);
  PRINT_SELF(vtkSMCompoundSourceProxyDefinitionBuilder);
  PRINT_SELF(vtkSMContextViewProxy);
  PRINT_SELF(vtkSMCSVExporterProxy);
  PRINT_SELF(vtkSMDataTypeDomain);
  PRINT_SELF(vtkSMDeserializer);
  PRINT_SELF(vtkSMDimensionsDomain);
  PRINT_SELF(vtkSMDocumentation);
  PRINT_SELF(vtkSMDomain);
  PRINT_SELF(vtkSMDomainIterator);
  PRINT_SELF(vtkSMDoubleRangeDomain);
  PRINT_SELF(vtkSMDoubleVectorProperty);
  PRINT_SELF(vtkSMEnumerationDomain);
  PRINT_SELF(vtkSMExporterProxy);
  PRINT_SELF(vtkSMExtentDomain);
  PRINT_SELF(vtkSMFieldDataDomain);
  PRINT_SELF(vtkSMFileListDomain);
  PRINT_SELF(vtkSMFixedTypeDomain);
  PRINT_SELF(vtkSMGlobalPropertiesLinkUndoElement);
  PRINT_SELF(vtkSMGlobalPropertiesProxy);
  PRINT_SELF(vtkSMIdTypeVectorProperty);
  PRINT_SELF(vtkSMImplicitPlaneRepresentationProxy);
  PRINT_SELF(vtkSMInputArrayDomain);
  PRINT_SELF(vtkSMInputProperty);
  PRINT_SELF(vtkSMInteractionUndoStackBuilder);
  PRINT_SELF(vtkSMIntRangeDomain);
  PRINT_SELF(vtkSMIntVectorProperty);
  PRINT_SELF(vtkSMLink);
  PRINT_SELF(vtkSMNamedPropertyIterator);
  PRINT_SELF(vtkSMNewWidgetRepresentationProxy);
  PRINT_SELF(vtkSMNullProxy);
  PRINT_SELF(vtkSMNumberOfComponentsDomain);
  PRINT_SELF(vtkSMObject);
  PRINT_SELF(vtkSMOrderedPropertyIterator);
  PRINT_SELF(vtkSMOutputPort);
  PRINT_SELF(vtkSMPipelineState);
  PRINT_SELF(vtkSMPluginLoaderProxy);
  PRINT_SELF(vtkSMPluginManager);
  PRINT_SELF(vtkSMProperty);
  // PRINT_SELF(vtkSMPropertyHelper);
  PRINT_SELF(vtkSMPropertyIterator);
  PRINT_SELF(vtkSMPropertyLink);
  PRINT_SELF(vtkSMPropertyModificationUndoElement);
  PRINT_SELF(vtkSMProxy);
  PRINT_SELF(vtkSMProxyConfigurationReader);
  PRINT_SELF(vtkSMProxyConfigurationWriter);
  PRINT_SELF(vtkSMProxyGroupDomain);
  PRINT_SELF(vtkSMProxyIterator);
  PRINT_SELF(vtkSMProxyLink);
  PRINT_SELF(vtkSMProxyListDomain);
  PRINT_SELF(vtkSMProxyLocator);
  // PRINT_SELF(vtkSMProxyManager);
  PRINT_SELF(vtkSMProxyProperty);
  PRINT_SELF(vtkSMProxySelectionModel);
  PRINT_SELF(vtkSMPSWriterProxy);
  PRINT_SELF(vtkSMPVRepresentationProxy);
  PRINT_SELF(vtkSMPWriterProxy);
  PRINT_SELF(vtkSMReaderFactory);
  PRINT_SELF(vtkSMRemoteObject);
  PRINT_SELF(vtkSMRemoteObjectUpdateUndoElement);
  PRINT_SELF(vtkSMRenderViewExporterProxy);
  PRINT_SELF(vtkSMRenderViewProxy);
  PRINT_SELF(vtkSMRepresentationProxy);
  PRINT_SELF(vtkSMScalarBarWidgetRepresentationProxy);
  PRINT_SELF(vtkSMSelectionHelper);
  // PRINT_SELF(vtkSMSession);
  PRINT_SELF(vtkSMSessionClient);
  PRINT_SELF(vtkSMSILDomain);
  PRINT_SELF(vtkSMSILModel);
  PRINT_SELF(vtkSMSourceProxy);
  PRINT_SELF(vtkSMSpreadSheetRepresentationProxy);
  PRINT_SELF(vtkSMStateLoader);
  PRINT_SELF(vtkSMStateLocator);
  PRINT_SELF(vtkSMStateVersionController);
  PRINT_SELF(vtkSMStringListDomain);
  PRINT_SELF(vtkSMStringVectorProperty);
  PRINT_SELF(vtkSMTesting);
  PRINT_SELF(vtkSMTextWidgetRepresentationProxy);
  PRINT_SELF(vtkSMTimeKeeper);
  PRINT_SELF(vtkSMTimeKeeperProxy);
  PRINT_SELF(vtkSMUndoElement);
  PRINT_SELF(vtkSMUndoStack);
  PRINT_SELF(vtkSMUndoStackBuilder);
  PRINT_SELF(vtkSMUtilities);
  PRINT_SELF(vtkSMVectorProperty);
  PRINT_SELF(vtkSMViewProxy);
  PRINT_SELF(vtkSMWidgetRepresentationProxy);
  PRINT_SELF(vtkSMWriterFactory);
  PRINT_SELF(vtkSMWriterProxy);

  return 0;
}
