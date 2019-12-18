#include "vtkPVConfig.h"

#define PRINT_SELF(classname)                                                                      \
  c = classname::New();                                                                            \
  c->Print(cout);                                                                                  \
  c->Delete();

#include "vtkCompositeAnimationPlayer.h"
#include "vtkPVBooleanKeyFrame.h"
#include "vtkPVCameraAnimationCue.h"
#include "vtkPVCameraCueManipulator.h"
#include "vtkPVCameraKeyFrame.h"
#include "vtkPVCompositeKeyFrame.h"
#include "vtkPVExponentialKeyFrame.h"
#include "vtkPVKeyFrame.h"
#include "vtkPVKeyFrameAnimationCueForProxies.h"
#include "vtkPVKeyFrameCueManipulator.h"
#include "vtkPVRampKeyFrame.h"
#include "vtkPVRepresentationAnimationHelper.h"
#include "vtkPVSinusoidKeyFrame.h"
#include "vtkRealtimeAnimationPlayer.h"
#include "vtkSIXMLAnimationWriterRepresentationProperty.h"
#include "vtkSMAnimationScene.h"
#include "vtkSMAnimationSceneGeometryWriter.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSequenceAnimationPlayer.h"
#include "vtkTimestepsAnimationPlayer.h"
#include "vtkXMLPVAnimationWriter.h"

int ParaViewCoreAnimationPrintSelf(int, char* [])
{
  vtkObject* c;
  PRINT_SELF(vtkCompositeAnimationPlayer);
  PRINT_SELF(vtkPVBooleanKeyFrame);
  PRINT_SELF(vtkPVCameraAnimationCue);
  PRINT_SELF(vtkPVCameraCueManipulator);
  PRINT_SELF(vtkPVCameraKeyFrame);
  PRINT_SELF(vtkPVCompositeKeyFrame);
  PRINT_SELF(vtkPVExponentialKeyFrame);
  PRINT_SELF(vtkPVKeyFrameAnimationCueForProxies);
  PRINT_SELF(vtkPVKeyFrameCueManipulator);
  PRINT_SELF(vtkPVKeyFrame);
  PRINT_SELF(vtkPVRampKeyFrame);
  PRINT_SELF(vtkPVRepresentationAnimationHelper);
  PRINT_SELF(vtkPVSinusoidKeyFrame);
  PRINT_SELF(vtkRealtimeAnimationPlayer);
  PRINT_SELF(vtkSequenceAnimationPlayer);
  PRINT_SELF(vtkSIXMLAnimationWriterRepresentationProperty);
  PRINT_SELF(vtkSMAnimationScene);
  PRINT_SELF(vtkSMAnimationSceneGeometryWriter);
  PRINT_SELF(vtkSMAnimationSceneProxy);
  PRINT_SELF(vtkTimestepsAnimationPlayer);
  PRINT_SELF(vtkXMLPVAnimationWriter);
  return 0;
}
