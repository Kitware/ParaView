/*=========================================================================

   Program: ParaView
   Module: pqStandardPropertyWidgetInterface.h

   Copyright (c) 2012 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#ifndef _pqStandardPropertyWidgetInterface_h
#define _pqStandardPropertyWidgetInterface_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetInterface.h"

/**
* pqStandardPropertyWidgetInterface provides a concrete implementation of
* pqPropertyWidget used by ParaView application. It adds logic to create some
* of the custom widgets and decorators used by ParaView's Properties Panel.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqStandardPropertyWidgetInterface
  : public QObject,
    public pqPropertyWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(pqPropertyWidgetInterface)
public:
  pqStandardPropertyWidgetInterface(QObject* p = 0);
  virtual ~pqStandardPropertyWidgetInterface();

  /**
  * Given a proxy and its property, create a widget for the same, if possible.
  * For unsupported/unknown proxies/properties, implementations should simply
  * return NULL without raising any errors (or messages).
  * Supported types are:
  * \li \c calculator : pqCalculatorWidget
  * \li \c camera_manipulator : pqCameraManipulatorWidget
  * \li \c color_palette_selector : pqColorPaletteSelectorWidget
  * \li \c color_selector : pqColorSelectorPropertyWidget
  * \li \c color_selector_with_palette : pqColorSelectorPropertyWidget with palette menu
  * \li \c command_button : pqCommandButtonPropertyWidget
  * \li \c display_representation_selector : pqDisplayRepresentationPropertyWidget
  * \li \c double_range : pqDoubleRangeSliderPropertyWidget
  * \li \c image_compressor_config : pqImageCompressorWidget
  * \li \c index_selection : pqIndexSelectionWidget
  * \li \c input_selector : pqInputSelectorWidget
  * \li \c list : pqListPropertyWidget
  * \li \c texture_selector : pqTextureSelectorPropertyWidget
  * \li \c transfer_function_editor : pqTransferFunctionWidgetPropertyWidget
  * \li \c viewtype_selector: pqViewTypePropertyWidget
  * \li \c glyph_scale_factor: pqGlyphScaleFactorPropertyWidget
  * \li \c proxy_editor: pqProxyEditorPropertyWidget
  * \li \c int_mask: pqIntMaskPropertyWidget
  * \li \c filename_widget: pqFileNamePropertyWidget
  * \li \c view_resolution: pqViewResolutionPropertyWidget
  */
  virtual pqPropertyWidget* createWidgetForProperty(vtkSMProxy* proxy, vtkSMProperty* property);

  /**
  * Given a proxy and its property group, create a widget for the same, of possible.
  * For unsupported/unknown proxies/property-groups, implementations should simply
  * return NULL without raising any errors (or messages).
  * Supported types are:
  * \li \c AnnotationsEditor : pqColorAnnotationsPropertyWidget
  * \li \c ArrayStatus : pqArrayStatusPropertyWidget
  * \li \c BackgroundEditor : pqBackgroundPropertyWidget
  * \li \c ColorEditor : pqColorEditorPropertyWidget
  * \li \c ColorOpacityEditor : pqColorOpacityEditorWidget
  * \li \c FontEditor : pqFontPropertyWidget
  * \li \c LightsEditor : pqLightsPropertyGroup
  * \li \c SeriesEditor : pqSeriesEditorPropertyWidget
  * \li \c InteractivePlane : pqImplicitPlanePropertyWidget
  * \li \c InteractiveBox: pqBoxPropertyWidget
  * \li \c InteractiveHandle: pqHandlePropertyWidget
  * \li \c InteractiveLine: pqLinePropertyWidget
  * \li \c InteractiveSpline: pqSplinePropertyWidget
  * \li \c InteractiveSphere: pqSpherePropertyWidget
  * \li \c InteractivePolyLine: pqSplinePropertyWidget (with mode==POLYLINE)
  * \li \c InteractiveCylinder: pqCylinderPropertyWidget
  * \li \c YoungsMaterial: pqYoungsMaterialPropertyWidget
  * \li \c cinema_export_selector : pqCinemaConfiguration
  */
  virtual pqPropertyWidget* createWidgetForPropertyGroup(
    vtkSMProxy* proxy, vtkSMPropertyGroup* group);

  /**
  * Given the type of the decorator and the pqPropertyWidget that needs to be
  * decorated, create the pqPropertyWidgetDecorator instance, if possible.
  * For unsupported/unknown, implementations should simply return NULL without
  * raising any errors (or messages).
  * Supported types are:
  * \li \c CTHArraySelectionDecorator : pqCTHArraySelectionDecorator
  * \li \c EnableWidgetDecorator : pqEnableWidgetDecorator
  * \li \c ShowWidgetDecorator : pqShowWidgetDecorator
  * \li \c InputDataTypeDecorator : pqInputDataTypeDecorator
  * \li \c GenericDecorator: pqGenericPropertyWidgetDecorator
  * \li \c OSPRayHidingDecorator: pqOSPRayHidingDecorator
  */
  virtual pqPropertyWidgetDecorator* createWidgetDecorator(
    const QString& type, vtkPVXMLElement* config, pqPropertyWidget* widget);
};

#endif // _pqStandardPropertyWidgetInterface_h
