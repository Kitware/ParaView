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


#include "vtkObject.h"

class VTK_EXPORT vtkKWEvent
{
  public:
  static const char* GetStringFromEventId(unsigned long event);
  static unsigned long GetEventIdFromString(const char* event);
  enum {
    KWWidgetEvents = 2000,
    MessageDialogInvokeEvent,
    FocusInEvent,
    FocusOutEvent,
    // VV    
    AngleVisibilityChangedEvent,
    AnnotationColorChangedEvent,
    ApplicationAreaChangedEvent,
    BackgroundColorChangedEvent,
    ContourAnnotationActiveChangedEvent,
    ContourAnnotationAddAtPositionEvent,
    ContourAnnotationAddEvent,
    ContourAnnotationColorByScalarsChangedEvent,
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
    GradientOpacityFunctionChangedEvent,
    GradientOpacityFunctionPresetApplyEvent,
    GradientOpacityStateChangedEvent,
    HistogramChangedEvent,
    ImageAngleVisibilityChangedEvent,
    ImageBackgroundColorChangedEvent,
    ImageCameraFocalPointAndPositionChangedEvent,
    ImageCameraResetEvent,
    ImageColorMappingEvent,
    ImageCornerAnnotationChangedEvent,
    ImageDistanceVisibilityChangedEvent,
    ImageHeaderAnnotationChangedEvent,
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
    MaterialPropertyChangingEvent,
    MiddleMouseOptionChangedEvent,
    MouseBindingChangedEvent,
    MouseOperationsChangedEvent,
    NotebookHidePageEvent,
    NotebookPinPageEvent,
    NotebookRaisePageEvent,
    NotebookShowPageEvent,
    NotebookUnpinPageEvent,
    ObliqueProbeColorChangedEvent,
    ObliqueProbeMovementEvent,
    ObliqueProbeResetEvent,
    ObliqueProbeScalarsVisibilityChangedEvent,
    ObliqueProbeVisibilityChangedEvent,
    PerspectiveViewAngleChangedEvent,
    PluginFilterApplyEvent,
    PluginFilterApplyPrepareEvent,
    PluginFilterCancelEvent,
    PluginFilterListEvent,
    PluginFilterListAddedEvent,
    PluginFilterListRemovedEvent,
    PluginFilterRedoEvent,
    PluginFilterRemoveMeshEvent,
    PluginFilterSelectEvent,
    PluginFilterUndoEvent,
    PrinterDPIChangedEvent,
    ProbeInformationChangedEvent,
    ProbeInformationOffEvent,
    ProjectionTypeChangedEvent,
    ReceiveRemoteSessionEvent,
    RenderEvent,
    RenderWidgetInSelectionFrameChangedEvent,
    RightMouseOptionChangedEvent,
    ScalarColorFunctionChangedEvent,
    ScalarColorFunctionPresetApplyEvent,
    ScalarComponentChangedEvent,
    ScalarComponentWeightChangedEvent,
    ScalarComponentWeightChangingEvent,
    ScalarOpacityFunctionChangedEvent,
    ScalarOpacityFunctionPresetApplyEvent,
    ScaleBarVisibilityChangedEvent,
    ShiftLeftMouseOptionChangedEvent,
    ShiftMiddleMouseOptionChangedEvent,
    ShiftRightMouseOptionChangedEvent,
    StandardInteractivityChangedEvent,
    SurfacePropertyChangedEvent,
    SurfacePropertyChangingEvent,
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
    VolumeHeaderAnnotationChangedEvent,
    VolumeMapperComputeGradientsStartEvent,
    VolumeMapperComputeGradientsEndEvent,
    VolumeMapperComputeGradientsProgressEvent,
    VolumeMapperRenderStartEvent,
    VolumeMapperRenderEndEvent,
    VolumeMapperRenderProgressEvent,
    VolumeMaterialPropertyChangedEvent,
    VolumeMaterialPropertyChangingEvent,
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
    VolumeScalarBarComponentChangedEvent,
    VolumeScalarBarWidgetChangedEvent,
    VolumeScaleBarColorChangedEvent,
    VolumeScaleBarVisibilityChangedEvent,
    VolumeStandardCameraViewEvent,
    VolumeZSamplingChangedEvent,
    WindowInterfaceChangedEvent,
    WindowLayoutChangedEvent,
    WindowLevelChangedEvent,
    WindowLevelChangingEndEvent,
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



