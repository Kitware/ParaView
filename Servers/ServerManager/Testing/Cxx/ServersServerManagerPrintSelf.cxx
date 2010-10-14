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

#include "vtkDummyProcessModuleHelper.h"
#include "vtkInitializationHelper.h"
#include "vtkPVBatchOptions.h"
#include "vtkSMAnimationCueManipulatorProxy.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMAnimationPlayerProxy.h"
#include "vtkSMAnimationSceneGeometryWriter.h"
#include "vtkSMAnimationSceneImageWriter.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMAnimationSceneWriter.h"
#include "vtkSMApplication.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMArrayListInformationHelper.h"
#include "vtkSMArrayRangeDomain.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMArraySelectionInformationHelper.h"
#include "vtkSMBooleanDomain.h"
#include "vtkSMBooleanKeyFrameProxy.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMBoxProxy.h"
#include "vtkSMBoxRepresentationProxy.h"
#include "vtkSMCameraConfigurationReader.h"
#include "vtkSMCameraConfigurationWriter.h"
#include "vtkSMCameraKeyFrameProxy.h"
#include "vtkSMCameraLink.h"
#include "vtkSMCameraManipulatorProxy.h"
#include "vtkSMCameraProxy.h"
#include "vtkSMChartRepresentationProxy.h"
#include "vtkSMComparativeAnimationCueProxy.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMCompositeKeyFrameProxy.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMCompoundProxyDefinitionLoader.h"
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMConnectionCleanerProxy.h"
#include "vtkSMContextArraysInformationHelper.h"
#include "vtkSMContextNamedOptionsProxy.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMCSVExporterProxy.h"
#include "vtkSMDataSourceProxy.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMDeserializer.h"
#include "vtkSMDimensionsDomain.h"
#include "vtkSMDistanceRepresentation2DProxy.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleArrayInformationHelper.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMExponentialKeyFrameProxy.h"
#include "vtkSMExporterProxy.h"
#include "vtkSMExtentDomain.h"
#include "vtkSMFetchDataProxy.h"
#include "vtkSMFieldDataDomain.h"
#include "vtkSMFileListDomain.h"
#include "vtkSMFileSeriesReaderProxy.h"
#include "vtkSMFixedTypeDomain.h"
#include "vtkSMGlobalPropertiesLinkUndoElement.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMHardwareSelector.h"
#include "vtkSMIdBasedProxyLocator.h"
#include "vtkSMIdTypeArrayInformationHelper.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMImageTextureProxy.h"
#include "vtkSMImplicitPlaneProxy.h"
#include "vtkSMImplicitPlaneRepresentationProxy.h"
#include "vtkSMInformationHelper.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntArrayInformationHelper.h"
#include "vtkSMInteractionUndoStackBuilder.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMKeyFrameAnimationCueManipulatorProxy.h"
#include "vtkSMKeyFrameProxy.h"
#include "vtkSMLinearAnimationCueManipulatorProxy.h"
#include "vtkSMLink.h"
#include "vtkSMLookupTableProxy.h"
#include "vtkSMMaterialLoaderProxy.h"
#include "vtkSMNamedPropertyIterator.h"
#include "vtkSMNetworkImageSourceProxy.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMNullProxy.h"
#include "vtkSMNumberOfComponentsDomain.h"
#include "vtkSMNumberOfGroupsDomain.h"
#include "vtkSMObject.h"
#include "vtkSMOrderedPropertyIterator.h"
#include "vtkSMOutputPort.h"
#include "vtkSMParallelCoordinatesRepresentationProxy.h"
#include "vtkSMPluginManager.h"
#include "vtkSMPluginProxy.h"
#include "vtkSMPQStateLoader.h"
#include "vtkSMPropertyAdaptor.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMPropertyModificationUndoElement.h"
#include "vtkSMProxyConfigurationReader.h"
#include "vtkSMProxyConfigurationWriter.h"
#include "vtkSMProxyDefinitionIterator.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyManagerReviver.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxyRegisterUndoElement.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMProxyStateChangedUndoElement.h"
#include "vtkSMProxyUnRegisterUndoElement.h"
#include "vtkSMPSWriterProxy.h"
#include "vtkSMPVLookupTableProxy.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPWriterProxy.h"
#include "vtkSMPythonTraceObserver.h"
#include "vtkSMRampKeyFrameProxy.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMRenderViewExporterProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationAnimationHelperProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMScalarBarActorProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSelectionRepresentationProxy.h"
#include "vtkSMServerFileListingProxy.h"
#include "vtkSMServerProxyManagerReviver.h"
#include "vtkSMServerSideAnimationPlayer.h"
#include "vtkSMSILDomain.h"
#include "vtkSMSILInformationHelper.h"
#include "vtkSMSILModel.h"
#include "vtkSMSimpleDoubleInformationHelper.h"
#include "vtkSMSimpleIdTypeInformationHelper.h"
#include "vtkSMSimpleIntInformationHelper.h"
#include "vtkSMSimpleStringInformationHelper.h"
#include "vtkSMSinusoidKeyFrameProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMSpreadSheetRepresentationProxy.h"
#include "vtkSMStateLoader.h"
#include "vtkSMStateVersionControllerBase.h"
#include "vtkSMStateVersionController.h"
#include "vtkSMStringArrayHelper.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringListRangeDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMSubPropertyIterator.h"
#include "vtkSMTesting.h"
#include "vtkSMTextSourceRepresentationProxy.h"
#include "vtkSMTextWidgetRepresentationProxy.h"
#include "vtkSMTimeAnimationCueProxy.h"
#include "vtkSMTimeKeeperProxy.h"
#include "vtkSMTimeRangeInformationHelper.h"
#include "vtkSMTimeStepsInformationHelper.h"
#include "vtkSMTransformProxy.h"
#include "vtkSMTwoDRenderViewProxy.h"
#include "vtkSMUndoElement.h"
#include "vtkSMUndoRedoStateLoader.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkSMUndoStack.h"
#include "vtkSMUniformGridVolumeRepresentationProxy.h"
#include "vtkSMUnstructuredGridVolumeRepresentationProxy.h"
#include "vtkSMUpdateInformationUndoElement.h"
#include "vtkSMUtilities.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMViewProxy.h"
#include "vtkSMWidgetRepresentationProxy.h"
#include "vtkSMWriterFactory.h"
#include "vtkSMWriterProxy.h"
#include "vtkSMXMLParser.h"
#include "vtkSMXMLPVAnimationWriterProxy.h"

#define PRINT_SELF(classname)\
  c = classname::New(); c->Print(cout); c->Delete();

int main(int , char *[])
{
  vtkObject *c;
  PRINT_SELF(vtkDummyProcessModuleHelper);
  PRINT_SELF(vtkInitializationHelper);
  PRINT_SELF(vtkPVBatchOptions);
  PRINT_SELF(vtkSMAnimationCueManipulatorProxy);
  PRINT_SELF(vtkSMAnimationCueProxy);
  PRINT_SELF(vtkSMAnimationPlayerProxy);
  PRINT_SELF(vtkSMAnimationSceneGeometryWriter);
  PRINT_SELF(vtkSMAnimationSceneImageWriter);
  PRINT_SELF(vtkSMAnimationSceneProxy);
  PRINT_SELF(vtkSMAnimationSceneWriter);
  PRINT_SELF(vtkSMApplication);
  PRINT_SELF(vtkSMArrayListDomain);
  PRINT_SELF(vtkSMArrayListInformationHelper);
  PRINT_SELF(vtkSMArrayRangeDomain);
  PRINT_SELF(vtkSMArraySelectionDomain);
  PRINT_SELF(vtkSMArraySelectionInformationHelper);
  PRINT_SELF(vtkSMBooleanDomain);
  PRINT_SELF(vtkSMBooleanKeyFrameProxy);
  PRINT_SELF(vtkSMBoundsDomain);
  PRINT_SELF(vtkSMBoxProxy);
  PRINT_SELF(vtkSMBoxRepresentationProxy);
  PRINT_SELF(vtkSMCameraConfigurationReader);
  PRINT_SELF(vtkSMCameraConfigurationWriter);
  PRINT_SELF(vtkSMCameraKeyFrameProxy);
  PRINT_SELF(vtkSMCameraLink);
  PRINT_SELF(vtkSMCameraManipulatorProxy);
  PRINT_SELF(vtkSMCameraProxy);
  PRINT_SELF(vtkSMChartRepresentationProxy);
  PRINT_SELF(vtkSMComparativeAnimationCueProxy);
  PRINT_SELF(vtkSMComparativeViewProxy);
  PRINT_SELF(vtkSMCompositeKeyFrameProxy);
  PRINT_SELF(vtkSMCompositeTreeDomain);
  PRINT_SELF(vtkSMCompoundProxyDefinitionLoader);
  PRINT_SELF(vtkSMCompoundSourceProxy);
  PRINT_SELF(vtkSMConnectionCleanerProxy);
  PRINT_SELF(vtkSMContextArraysInformationHelper);
  PRINT_SELF(vtkSMContextNamedOptionsProxy);
  PRINT_SELF(vtkSMContextViewProxy);
  PRINT_SELF(vtkSMCSVExporterProxy);
  PRINT_SELF(vtkSMDataSourceProxy);
  PRINT_SELF(vtkSMDataTypeDomain);
  PRINT_SELF(vtkSMDeserializer);
  PRINT_SELF(vtkSMDimensionsDomain);
  PRINT_SELF(vtkSMDistanceRepresentation2DProxy);
  PRINT_SELF(vtkSMDocumentation);
  PRINT_SELF(vtkSMDomain);
  PRINT_SELF(vtkSMDomainIterator);
  PRINT_SELF(vtkSMDoubleArrayInformationHelper);
  PRINT_SELF(vtkSMDoubleRangeDomain);
  PRINT_SELF(vtkSMDoubleVectorProperty);
  PRINT_SELF(vtkSMEnumerationDomain);
  PRINT_SELF(vtkSMExponentialKeyFrameProxy);
  PRINT_SELF(vtkSMExporterProxy);
  PRINT_SELF(vtkSMExtentDomain);
  PRINT_SELF(vtkSMFetchDataProxy);
  PRINT_SELF(vtkSMFieldDataDomain);
  PRINT_SELF(vtkSMFileListDomain);
  PRINT_SELF(vtkSMFileSeriesReaderProxy);
  PRINT_SELF(vtkSMFixedTypeDomain);
  PRINT_SELF(vtkSMGlobalPropertiesLinkUndoElement);
  PRINT_SELF(vtkSMGlobalPropertiesManager);
  PRINT_SELF(vtkSMHardwareSelector);
  PRINT_SELF(vtkSMIdBasedProxyLocator);
  PRINT_SELF(vtkSMIdTypeArrayInformationHelper);
  PRINT_SELF(vtkSMIdTypeVectorProperty);
  PRINT_SELF(vtkSMImageTextureProxy);
  PRINT_SELF(vtkSMImplicitPlaneProxy);
  PRINT_SELF(vtkSMImplicitPlaneRepresentationProxy);
  PRINT_SELF(vtkSMInformationHelper);
  PRINT_SELF(vtkSMInputArrayDomain);
  PRINT_SELF(vtkSMInputProperty);
  PRINT_SELF(vtkSMIntArrayInformationHelper);
  PRINT_SELF(vtkSMInteractionUndoStackBuilder);
  PRINT_SELF(vtkSMIntRangeDomain);
  PRINT_SELF(vtkSMIntVectorProperty);
  PRINT_SELF(vtkSMKeyFrameAnimationCueManipulatorProxy);
  PRINT_SELF(vtkSMKeyFrameProxy);
  PRINT_SELF(vtkSMLinearAnimationCueManipulatorProxy);
  PRINT_SELF(vtkSMLink);
  PRINT_SELF(vtkSMLookupTableProxy);
  PRINT_SELF(vtkSMMaterialLoaderProxy);
  PRINT_SELF(vtkSMNamedPropertyIterator);
  PRINT_SELF(vtkSMNetworkImageSourceProxy);
  PRINT_SELF(vtkSMNewWidgetRepresentationProxy);
  PRINT_SELF(vtkSMNullProxy);
  PRINT_SELF(vtkSMNumberOfComponentsDomain);
  PRINT_SELF(vtkSMNumberOfGroupsDomain);
  PRINT_SELF(vtkSMObject);
  PRINT_SELF(vtkSMOrderedPropertyIterator);
  PRINT_SELF(vtkSMOutputPort);
  PRINT_SELF(vtkSMParallelCoordinatesRepresentationProxy);
  PRINT_SELF(vtkSMPluginManager);
  PRINT_SELF(vtkSMPluginProxy);
  PRINT_SELF(vtkSMPQStateLoader);
  PRINT_SELF(vtkSMPropertyAdaptor);
  PRINT_SELF(vtkSMProperty);
  PRINT_SELF(vtkSMPropertyIterator);
  PRINT_SELF(vtkSMPropertyLink);
  PRINT_SELF(vtkSMPropertyModificationUndoElement);
  PRINT_SELF(vtkSMProxyConfigurationReader);
  PRINT_SELF(vtkSMProxyConfigurationWriter);
  PRINT_SELF(vtkSMProxyDefinitionIterator);
  PRINT_SELF(vtkSMProxyGroupDomain);
  PRINT_SELF(vtkSMProxy);
  PRINT_SELF(vtkSMProxyIterator);
  PRINT_SELF(vtkSMProxyLink);
  PRINT_SELF(vtkSMProxyListDomain);
  PRINT_SELF(vtkSMProxyLocator);
  PRINT_SELF(vtkSMProxyManager);
  PRINT_SELF(vtkSMProxyManagerReviver);
  PRINT_SELF(vtkSMProxyProperty);
  PRINT_SELF(vtkSMProxyRegisterUndoElement);
  PRINT_SELF(vtkSMProxySelectionModel);
  PRINT_SELF(vtkSMProxyStateChangedUndoElement);
  PRINT_SELF(vtkSMProxyUnRegisterUndoElement);
  PRINT_SELF(vtkSMPSWriterProxy);
  PRINT_SELF(vtkSMPVLookupTableProxy);
  PRINT_SELF(vtkSMPVRepresentationProxy);
  PRINT_SELF(vtkSMPWriterProxy);
  PRINT_SELF(vtkSMPythonTraceObserver);
  PRINT_SELF(vtkSMRampKeyFrameProxy);
  PRINT_SELF(vtkSMReaderFactory);
  PRINT_SELF(vtkSMRenderViewExporterProxy);
  PRINT_SELF(vtkSMRenderViewProxy);
  PRINT_SELF(vtkSMRepresentationAnimationHelperProxy);
  PRINT_SELF(vtkSMRepresentationProxy);
  PRINT_SELF(vtkSMScalarBarActorProxy);
  PRINT_SELF(vtkSMScalarBarWidgetRepresentationProxy);
  PRINT_SELF(vtkSMSelectionHelper);
  PRINT_SELF(vtkSMSelectionRepresentationProxy);
  PRINT_SELF(vtkSMServerFileListingProxy);
  PRINT_SELF(vtkSMServerProxyManagerReviver);
  PRINT_SELF(vtkSMServerSideAnimationPlayer);
  PRINT_SELF(vtkSMSILDomain);
  PRINT_SELF(vtkSMSILInformationHelper);
  PRINT_SELF(vtkSMSILModel);
  PRINT_SELF(vtkSMSimpleDoubleInformationHelper);
  PRINT_SELF(vtkSMSimpleIdTypeInformationHelper);
  PRINT_SELF(vtkSMSimpleIntInformationHelper);
  PRINT_SELF(vtkSMSimpleStringInformationHelper);
  PRINT_SELF(vtkSMSinusoidKeyFrameProxy);
  PRINT_SELF(vtkSMSourceProxy);
  PRINT_SELF(vtkSMSpreadSheetRepresentationProxy);
  PRINT_SELF(vtkSMStateLoader);
  PRINT_SELF(vtkSMStateVersionControllerBase);
  PRINT_SELF(vtkSMStateVersionController);
  PRINT_SELF(vtkSMStringArrayHelper);
  PRINT_SELF(vtkSMStringListDomain);
  PRINT_SELF(vtkSMStringListRangeDomain);
  PRINT_SELF(vtkSMStringVectorProperty);
  PRINT_SELF(vtkSMSubPropertyIterator);
  PRINT_SELF(vtkSMTesting);
  PRINT_SELF(vtkSMTextSourceRepresentationProxy);
  PRINT_SELF(vtkSMTextWidgetRepresentationProxy);
  PRINT_SELF(vtkSMTimeAnimationCueProxy);
  PRINT_SELF(vtkSMTimeKeeperProxy);
  PRINT_SELF(vtkSMTimeRangeInformationHelper);
  PRINT_SELF(vtkSMTimeStepsInformationHelper);
  PRINT_SELF(vtkSMTransformProxy);
  PRINT_SELF(vtkSMTwoDRenderViewProxy);
  PRINT_SELF(vtkSMUndoElement);
  PRINT_SELF(vtkSMUndoRedoStateLoader);
  PRINT_SELF(vtkSMUndoStackBuilder);
  PRINT_SELF(vtkSMUndoStack);
  PRINT_SELF(vtkSMUniformGridVolumeRepresentationProxy);
  PRINT_SELF(vtkSMUnstructuredGridVolumeRepresentationProxy);
  PRINT_SELF(vtkSMUpdateInformationUndoElement);
  PRINT_SELF(vtkSMUtilities);
  PRINT_SELF(vtkSMVectorProperty);
  PRINT_SELF(vtkSMViewProxy);
  PRINT_SELF(vtkSMWidgetRepresentationProxy);
  PRINT_SELF(vtkSMWriterFactory);
  PRINT_SELF(vtkSMWriterProxy);
  PRINT_SELF(vtkSMXMLParser);
  PRINT_SELF(vtkSMXMLPVAnimationWriterProxy);
  return 0;
}
