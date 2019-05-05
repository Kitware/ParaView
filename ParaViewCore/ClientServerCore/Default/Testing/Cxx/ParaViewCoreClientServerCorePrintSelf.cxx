#include "vtkPVConfig.h"

#include "vtk3DWidgetRepresentation.h"
#include "vtkCaveSynchronizedRenderers.h"
#include "vtkChartRepresentation.h"
#include "vtkClientServerMoveData.h"
#include "vtkCompleteArrays.h"
#include "vtkCompositeRepresentation.h"
#include "vtkDataLabelRepresentation.h"
#include "vtkGeometryRepresentation.h"
#include "vtkGeometryRepresentationWithFaces.h"
#include "vtkGlyph3DRepresentation.h"
#include "vtkImageSliceMapper.h"
#include "vtkImageSliceRepresentation.h"
#include "vtkImageVolumeRepresentation.h"
#include "vtkMPIMToNSocketConnection.h"
#include "vtkMPIMToNSocketConnectionPortInformation.h"
#include "vtkMPIMoveData.h"
#include "vtkNetworkAccessManager.h"
#include "vtkNetworkImageSource.h"
#include "vtkOutlineRepresentation.h"
#include "vtkPVAlgorithmPortsInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVCacheKeeperPipeline.h"
#include "vtkPVCacheSizeInformation.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVClientServerSynchronizedRenderers.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVCompositeDataInformationIterator.h"
#include "vtkPVCompositeRepresentation.h"
#include "vtkPVContextView.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVDataRepresentationPipeline.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVDataSizeInformation.h"
#include "vtkPVEnvironmentInformation.h"
#include "vtkPVEnvironmentInformationHelper.h"
#include "vtkPVExtractSelection.h"
#include "vtkPVFileInformation.h"
#include "vtkPVFileInformationHelper.h"
#include "vtkPVGenericAttributeInformation.h"
#include "vtkPVImplicitPlaneRepresentation.h"
#include "vtkPVInformation.h"
#include "vtkPVLastSelectionInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVOptionsXMLParser.h"
#include "vtkPVParallelCoordinatesRepresentation.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVPluginsInformation.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVPythonModule.h"
#include "vtkPVPythonPluginInterface.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkPVRepresentedDataInformation.h"
#include "vtkPVSILInformation.h"
#include "vtkPVSelectionInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkPVServerManagerPluginInterface.h"
#include "vtkPVServerOptions.h"
#include "vtkPVSession.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkPVTimerInformation.h"
#include "vtkPVView.h"
#include "vtkPVXYChartView.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleAutoMPI.h"
#include "vtkSelectionDeliveryFilter.h"
#include "vtkSelectionRepresentation.h"
#include "vtkSession.h"
#include "vtkSpreadSheetRepresentation.h"
#include "vtkSpreadSheetView.h"
#include "vtkTCPNetworkAccessManager.h"
#include "vtkTextSourceRepresentation.h"
#include "vtkUnstructuredGridVolumeRepresentation.h"
#include "vtkXYChartRepresentation.h"

#if VTK_MODULE_ENABLE_ParaView_icet
#include "vtkIceTSynchronizedRenderers.h"
#endif

#define PRINT_SELF(classname)                                                                      \
  cout << "------------------------------------" << endl;                                          \
  cout << "Class: " << #classname << endl;                                                         \
  c = classname::New();                                                                            \
  c->Print(cout);                                                                                  \
  c->Delete();

int ParaViewCoreClientServerCorePrintSelf(int, char* [])
{
  vtkObject* c;
  PRINT_SELF(vtk3DWidgetRepresentation);
  // PRINT_SELF(vtkCaveSynchronizedRenderers);
  PRINT_SELF(vtkChartRepresentation);
  PRINT_SELF(vtkClientServerMoveData);
  PRINT_SELF(vtkCompleteArrays);
  PRINT_SELF(vtkCompositeRepresentation);
  PRINT_SELF(vtkDataLabelRepresentation);
  PRINT_SELF(vtkGeometryRepresentation);
  PRINT_SELF(vtkGeometryRepresentationWithFaces);
  PRINT_SELF(vtkGlyph3DRepresentation);
  PRINT_SELF(vtkImageSliceMapper);
  PRINT_SELF(vtkImageSliceRepresentation);
  PRINT_SELF(vtkImageVolumeRepresentation);
  PRINT_SELF(vtkMPIMToNSocketConnection);
  PRINT_SELF(vtkMPIMToNSocketConnectionPortInformation);
  PRINT_SELF(vtkMPIMoveData);
  PRINT_SELF(vtkNetworkAccessManager);
  PRINT_SELF(vtkNetworkImageSource);
  PRINT_SELF(vtkOutlineRepresentation);
  PRINT_SELF(vtkPVAlgorithmPortsInformation);
  PRINT_SELF(vtkPVArrayInformation);
  PRINT_SELF(vtkPVCacheKeeper);
  PRINT_SELF(vtkPVCacheKeeperPipeline);
  PRINT_SELF(vtkPVCacheSizeInformation);
  PRINT_SELF(vtkPVClassNameInformation);
  PRINT_SELF(vtkPVClientServerSynchronizedRenderers);
  PRINT_SELF(vtkPVCompositeDataInformation);
  PRINT_SELF(vtkPVCompositeDataInformationIterator);
  PRINT_SELF(vtkPVCompositeRepresentation);
  // PRINT_SELF(vtkPVContextView);
  PRINT_SELF(vtkPVDataInformation);
  PRINT_SELF(vtkPVDataRepresentation);
  PRINT_SELF(vtkPVDataRepresentationPipeline);
  PRINT_SELF(vtkPVDataSetAttributesInformation);
  PRINT_SELF(vtkPVDataSizeInformation);
  PRINT_SELF(vtkPVRenderingCapabilitiesInformation);
  PRINT_SELF(vtkPVEnvironmentInformation);
  PRINT_SELF(vtkPVEnvironmentInformationHelper);
  PRINT_SELF(vtkPVExtractSelection);
  PRINT_SELF(vtkPVFileInformation);
  PRINT_SELF(vtkPVFileInformationHelper);
  PRINT_SELF(vtkPVGenericAttributeInformation);
  PRINT_SELF(vtkPVImplicitPlaneRepresentation);
  PRINT_SELF(vtkPVInformation);
  PRINT_SELF(vtkPVLastSelectionInformation);
  PRINT_SELF(vtkPVOptions);
  PRINT_SELF(vtkPVOptionsXMLParser);
  PRINT_SELF(vtkPVParallelCoordinatesRepresentation);
  // PRINT_SELF(vtkPVPlugin);
  PRINT_SELF(vtkPVPluginLoader);
  PRINT_SELF(vtkPVPluginTracker);
  PRINT_SELF(vtkPVPluginsInformation);
  PRINT_SELF(vtkPVProgressHandler);
  // PRINT_SELF(vtkPVPythonModule);
  // PRINT_SELF(vtkPVPythonPluginInterface);
  // PRINT_SELF(vtkPVRenderView);
  PRINT_SELF(vtkPVRepresentedDataInformation);
  PRINT_SELF(vtkPVSILInformation);
  PRINT_SELF(vtkPVSelectionInformation);
  PRINT_SELF(vtkPVServerInformation);
  // PRINT_SELF(vtkPVServerManagerPluginInterface);
  PRINT_SELF(vtkPVServerOptions);
  PRINT_SELF(vtkPVSession);
  // PRINT_SELF(vtkPVSynchronizedRenderer);
  PRINT_SELF(vtkPVTemporalDataInformation);
  PRINT_SELF(vtkPVTimerInformation);
  // PRINT_SELF(vtkPVView);
  // PRINT_SELF(vtkPVXYChartView);
  PRINT_SELF(vtkProcessModule);
  PRINT_SELF(vtkProcessModuleAutoMPI);
  PRINT_SELF(vtkSelectionDeliveryFilter);
  PRINT_SELF(vtkSelectionRepresentation);
  PRINT_SELF(vtkSession);
  // PRINT_SELF(vtkSessionIterator); Requires process module to have been created.
  PRINT_SELF(vtkSpreadSheetRepresentation);
  // PRINT_SELF(vtkSpreadSheetView);
  PRINT_SELF(vtkTCPNetworkAccessManager);
  PRINT_SELF(vtkTextSourceRepresentation);
  PRINT_SELF(vtkUnstructuredGridVolumeRepresentation);
  PRINT_SELF(vtkXYChartRepresentation);

#if VTK_MODULE_ENABLE_ParaView_icet
  PRINT_SELF(vtkIceTSynchronizedRenderers);
#endif

  return 0;
}
