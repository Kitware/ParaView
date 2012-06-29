#include "vtkPVConfig.h"

#define PRINT_SELF(classname)\
  c = classname::New(); c->Print(cout); c->Delete();

#include "vtkSIArraySelectionProperty.h"
#include "vtkPVSessionBase.h"
#include "vtkPVSessionCore.h"
#include "vtkPVSessionCoreInterpreterHelper.h"
#include "vtkPVSessionServer.h"
#include "vtkSIChartRepresentationProxy.h"
#include "vtkSICompoundSourceProxy.h"
#include "vtkSIContextArraysProperty.h"
#include "vtkSIDataArrayProperty.h"
#include "vtkSIDoubleVectorProperty.h"
#include "vtkSIFileSeriesReaderProxy.h"
#include "vtkSIIdTypeVectorProperty.h"
#include "vtkSIImageTextureProxy.h"
#include "vtkSIInputProperty.h"
#include "vtkSIIntVectorProperty.h"
#include "vtkSIObject.h"
#include "vtkSIProperty.h"
#include "vtkSIProxyDefinitionManager.h"
#include "vtkSIProxy.h"
#include "vtkSIProxyProperty.h"
#include "vtkSIPVRepresentationProxy.h"
#include "vtkSIScalarBarActorProxy.h"
#include "vtkSISelectionRepresentationProxy.h"
#include "vtkSISILProperty.h"
#include "vtkSISourceProxy.h"
#include "vtkSIStringVectorProperty.h"
#include "vtkSITextSourceRepresentationProxy.h"
#include "vtkSITimeRangeProperty.h"
#include "vtkSITimeStepsProperty.h"
#include "vtkSIUniformGridVolumeRepresentationProxy.h"
#include "vtkSIUnstructuredGridVolumeRepresentationProxy.h"
#include "vtkSIVectorProperty.h"
#include "vtkSIWriterProxy.h"
#include "vtkSIXMLAnimationWriterRepresentationProperty.h"
#include "vtkSMMessage.h"


int main(int, char**)
{
  vtkObject* c;
  PRINT_SELF(vtkSIArraySelectionProperty);
  PRINT_SELF(vtkSIChartRepresentationProxy);
  PRINT_SELF(vtkSICompoundSourceProxy);
  PRINT_SELF(vtkSIContextArraysProperty);
  PRINT_SELF(vtkSIDataArrayProperty);
  PRINT_SELF(vtkSIDoubleVectorProperty);
  PRINT_SELF(vtkSIFileSeriesReaderProxy);
  PRINT_SELF(vtkSIIdTypeVectorProperty);
  PRINT_SELF(vtkSIImageTextureProxy);
  PRINT_SELF(vtkSIInputProperty);
  PRINT_SELF(vtkSIIntVectorProperty);
  PRINT_SELF(vtkSIObject);
  PRINT_SELF(vtkSIPVRepresentationProxy);
  PRINT_SELF(vtkSIProperty);
  PRINT_SELF(vtkSIProxy);
  PRINT_SELF(vtkSIProxyProperty);
  PRINT_SELF(vtkSISILProperty);
  PRINT_SELF(vtkSIScalarBarActorProxy);
  PRINT_SELF(vtkSISelectionRepresentationProxy);
  PRINT_SELF(vtkSISourceProxy);
  PRINT_SELF(vtkSIStringVectorProperty);
  PRINT_SELF(vtkSITextSourceRepresentationProxy);
  PRINT_SELF(vtkSITimeRangeProperty);
  PRINT_SELF(vtkSITimeStepsProperty);
  PRINT_SELF(vtkSIUniformGridVolumeRepresentationProxy);
  PRINT_SELF(vtkSIUnstructuredGridVolumeRepresentationProxy);
  PRINT_SELF(vtkSIVectorProperty);
  PRINT_SELF(vtkSIWriterProxy);
  PRINT_SELF(vtkSIXMLAnimationWriterRepresentationProperty);
  //PRINT_SELF(vtkSMMessage);
  PRINT_SELF(vtkSIProxyDefinitionManager);
  PRINT_SELF(vtkPVSessionBase);
  PRINT_SELF(vtkPVSessionCore);
  PRINT_SELF(vtkPVSessionCoreInterpreterHelper);
  PRINT_SELF(vtkPVSessionServer);
  return 0;
}
