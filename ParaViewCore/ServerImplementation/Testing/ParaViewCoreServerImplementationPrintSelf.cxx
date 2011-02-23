#include "vtkPVConfig.h"

#define PRINT_SELF(classname)\
  c = classname::New(); c->Print(cout); c->Delete();

#include "vtkPMArraySelectionProperty.h"
#include "vtkPMChartRepresentationProxy.h"
#include "vtkPMCompoundSourceProxy.h"
#include "vtkPMContextArraysProperty.h"
#include "vtkPMDataArrayProperty.h"
#include "vtkPMDoubleVectorProperty.h"
#include "vtkPMFileSeriesReaderProxy.h"
#include "vtkPMIdTypeVectorProperty.h"
#include "vtkPMImageTextureProxy.h"
#include "vtkPMInputProperty.h"
#include "vtkPMIntVectorProperty.h"
#include "vtkPMObject.h"
#include "vtkPMPVRepresentationProxy.h"
#include "vtkPMProperty.h"
#include "vtkPMProxy.h"
#include "vtkPMProxyProperty.h"
#include "vtkPMSILProperty.h"
#include "vtkPMScalarBarActorProxy.h"
#include "vtkPMSelectionRepresentationProxy.h"
#include "vtkPMSourceProxy.h"
#include "vtkPMStringVectorProperty.h"
#include "vtkPMTextSourceRepresentationProxy.h"
#include "vtkPMTimeRangeProperty.h"
#include "vtkPMTimeStepsProperty.h"
#include "vtkPMUniformGridVolumeRepresentationProxy.h"
#include "vtkPMUnstructuredGridVolumeRepresentationProxy.h"
#include "vtkPMVectorProperty.h"
#include "vtkPMWriterProxy.h"
#include "vtkPMXMLAnimationWriterRepresentationProperty.h"
#include "vtkSMMessage.h"
#include "vtkSMProxyDefinitionIterator.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSessionBase.h"
#include "vtkSMSessionCore.h"
#include "vtkSMSessionCoreInterpreterHelper.h"
#include "vtkSMSessionServer.h"


int main(int, char**)
{
  vtkObject* c;
  PRINT_SELF(vtkPMArraySelectionProperty);
  PRINT_SELF(vtkPMChartRepresentationProxy);
  PRINT_SELF(vtkPMCompoundSourceProxy);
  PRINT_SELF(vtkPMContextArraysProperty);
  PRINT_SELF(vtkPMDataArrayProperty);
  PRINT_SELF(vtkPMDoubleVectorProperty);
  PRINT_SELF(vtkPMFileSeriesReaderProxy);
  PRINT_SELF(vtkPMIdTypeVectorProperty);
  PRINT_SELF(vtkPMImageTextureProxy);
  PRINT_SELF(vtkPMInputProperty);
  PRINT_SELF(vtkPMIntVectorProperty);
  PRINT_SELF(vtkPMObject);
  PRINT_SELF(vtkPMPVRepresentationProxy);
  PRINT_SELF(vtkPMProperty);
  PRINT_SELF(vtkPMProxy);
  PRINT_SELF(vtkPMProxyProperty);
  PRINT_SELF(vtkPMSILProperty);
  PRINT_SELF(vtkPMScalarBarActorProxy);
  PRINT_SELF(vtkPMSelectionRepresentationProxy);
  PRINT_SELF(vtkPMSourceProxy);
  PRINT_SELF(vtkPMStringVectorProperty);
  PRINT_SELF(vtkPMTextSourceRepresentationProxy);
  PRINT_SELF(vtkPMTimeRangeProperty);
  PRINT_SELF(vtkPMTimeStepsProperty);
  PRINT_SELF(vtkPMUniformGridVolumeRepresentationProxy);
  PRINT_SELF(vtkPMUnstructuredGridVolumeRepresentationProxy);
  PRINT_SELF(vtkPMVectorProperty);
  PRINT_SELF(vtkPMWriterProxy);
  PRINT_SELF(vtkPMXMLAnimationWriterRepresentationProperty);
  //PRINT_SELF(vtkSMMessage);
  PRINT_SELF(vtkSMProxyDefinitionIterator);
  PRINT_SELF(vtkSMProxyDefinitionManager);
  PRINT_SELF(vtkSMSessionBase);
  PRINT_SELF(vtkSMSessionCore);
  PRINT_SELF(vtkSMSessionCoreInterpreterHelper);
  PRINT_SELF(vtkSMSessionServer);
  return 0;
}
