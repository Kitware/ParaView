/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkKWEvent - Event enumeration.

#ifndef __vtkKWEvent_h
#define __vtkKWEvent_h


#include "vtkKWObject.h"

class VTK_EXPORT vtkKWEvent
{
  public:
  static const char* GetStringFromEventId(unsigned long event);
  static unsigned long GetEventIdFromString(const char* event);
  enum {
    KWWidgetEvents = 2000,
    MessageDialogInvokeEvent,
    // VV    
    AngleVisibilityChangedEvent,
    AnnotationColorChangedEvent,
    ApplicationAreaChangedEvent,
    BackgroundColorChangedEvent,
    ColorImageEvent,
    ContourAnnotationActiveChangedEvent,
    ContourAnnotationAddAtPositionEvent,
    ContourAnnotationAddEvent,
    ContourAnnotationColorChangedEvent,
    ContourAnnotationComputeStatisticsEvent,
    ContourAnnotationLineWidthChangedEvent,
    ContourAnnotationOpacityChangedEvent,
    ContourAnnotationRemoveAllEvent,
    ContourAnnotationRemoveEvent,
    ContourAnnotationSurfacePropertyChangedEvent,
    ContourAnnotationSurfaceQualityChangedEvent,
    ContourAnnotationSurfaceRepresentationChangedEvent,
    ContourAnnotationVisibilityChangedEvent,
    ControlLeftMouseOptionChangedEvent,
    ControlMiddleMouseOptionChangedEvent,
    ControlRightMouseOptionChangedEvent,
    CroppingPlanesPositionChangedEvent,
    CroppingRegionFlagsChangedEvent,
    CroppingRegionsVisibilityChangedEvent,
    Cursor3DInteractiveStateChangedEvent,
    Cursor3DPositionChangedEvent,
    Cursor3DPositionChangingEvent,
    Cursor3DTypeChangedEvent,
    Cursor3DVisibilityChangedEvent,
    Cursor3DXColorChangedEvent,
    Cursor3DYColorChangedEvent,
    Cursor3DZColorChangedEvent,
    DistanceVisibilityChangedEvent,
    EnableShadingEvent,
    FilterRedoEvent,
    FilterRemoveMeshEvent,
    FilterUndoEvent,
    GradientOpacityFunctionChangedEvent,
    GradientOpacityFunctionPresetApplyEvent,
    GradientOpacityStateChangedEvent,
    GrayScaleImageEvent,
    HistogramChangedEvent,
    ImageAngleVisibilityChangedEvent,
    ImageBackgroundColorChangedEvent,
    ImageCameraFocalPointAndPositionChangedEvent,
    ImageCameraResetEvent,
    ImageCornerAnnotationChangedEvent,
    ImageDistanceVisibilityChangedEvent,
    ImageHeaderAnnotationColorChangedEvent,
    ImageHeaderAnnotationTextChangedEvent,
    ImageHeaderAnnotationVisibilityChangedEvent,
    ImageMouseBindingChangedEvent,
    ImageScaleBarColorChangedEvent,
    ImageScaleBarVisibilityChangedEvent,
    ImageSliceChangedEvent,
    ImageZoomFactorChangedEvent,
    InteractiveRenderStartEvent,
    LeftMouseOptionChangedEvent,
    LightActiveChangedEvent,
    LightColorChangedEvent,
    LightIntensityChangedEvent,
    LightPositionChangedEvent,
    LightVisibilityChangedEvent,
    LightboxOrientationChangedEvent,
    LightboxResolutionChangedEvent,
    Marker2DColorChangedEvent,
    Marker2DPositionChangedEvent,
    Marker2DVisibilityChangedEvent,
    Marker3DAddMarkerEvent,
    Marker3DColorChangedEvent,
    Marker3DPositionChangedEvent,
    Marker3DRemoveAllMarkersEvent,
    Marker3DRemoveSelectedMarkerEvent,
    Marker3DVisibilityChangedEvent,
    MaterialPropertyChangedEvent,
    MiddleMouseOptionChangedEvent,
    MouseBindingChangedEvent,
    MouseOperationsChangedEvent,
    MultiPassStillRenderEndEvent,
    MultiPassStillRenderStageChangedEvent,
    ObliqueProbeColorChangedEvent,
    ObliqueProbeMovementEvent,
    ObliqueProbeResetEvent,
    ObliqueProbeScalarsVisibilityChangedEvent,
    ObliqueProbeVisibilityChangedEvent,
    PerspectiveViewAngleChangedEvent,
    PluginFilterApplyEvent,
    PluginFilterCancelEvent,
    PluginFilterSelectEvent,
    PrinterDPIChangedEvent,
    ProbeInformationChangedEvent,
    ProjectionTypeChangedEvent,
    RenderEvent,
    RightMouseOptionChangedEvent,
    ScalarColorFunctionChangedEvent,
    ScalarColorFunctionPresetApplyEvent,
    ScalarComponentWeightingChangedEvent,
    ScalarOpacityFunctionChangedEvent,
    ScalarOpacityFunctionPresetApplyEvent,
    ScaleBarVisibilityChangedEvent,
    ShiftLeftMouseOptionChangedEvent,
    ShiftMiddleMouseOptionChangedEvent,
    ShiftRightMouseOptionChangedEvent,
    StandardInteractivityChangedEvent,
    SurfacePropertyChangedEvent,
    SwitchToVolumeProEvent,
    TransferFunctionsChangedEvent,
    TransferFunctionsChangingEvent,
    UserInterfaceVisibilityChangedEvent,
    ViewAnnotationChangedEvent,
    ViewSelectedEvent,
    VolumeBackgroundColorChangedEvent,
    VolumeBlendModeChangedEvent,
    VolumeBoundingBoxColorChangedEvent,
    VolumeBoundingBoxVisibilityChangedEvent,
    VolumeCameraResetEvent,
    VolumeCornerAnnotationChangedEvent,
    VolumeDistanceVisibilityChangedEvent,
    VolumeFlySpeedChangedEvent,
    VolumeHeaderAnnotationColorChangedEvent,
    VolumeHeaderAnnotationTextChangedEvent,
    VolumeHeaderAnnotationVisibilityChangedEvent,
    VolumeMaterialPropertyChangedEvent,
    VolumeMouseBindingChangedEvent,
    VolumeOrientationMarkerColorChangedEvent,
    VolumeOrientationMarkerVisibilityChangedEvent,
    VolumePropertyChangedEvent,
    VolumePropertyChangingEvent,
    VolumeReformatBoxVisibilityChangedEvent,
    VolumeReformatChangedEvent,
    VolumeReformatManipulationStyleChangedEvent,
    VolumeReformatPlaneChangedEvent,
    VolumeReformatThicknessChangedEvent,
    VolumeScalarBarColorChangedEvent,
    VolumeScalarBarTitleChangedEvent,
    VolumeScalarBarVisibilityChangedEvent,
    VolumeScaleBarColorChangedEvent,
    VolumeScaleBarVisibilityChangedEvent,
    VolumeStandardCameraViewEvent,
    VolumeZSamplingChangedEvent,
    WindowInterfaceChangedEvent,
    WindowLayoutChangedEvent,
    WindowLevelChangedEvent,
    WindowLevelChangingEvent,
    WindowLevelResetEvent,
    // PV
    ErrorMessageEvent,
    InitializeTraceEvent,
    ManipulatorModifiedEvent,
    WarningMessageEvent,
    WidgetModifiedEvent,
    //
    FinalBogusNotUsedEvent
  };
};

#endif



