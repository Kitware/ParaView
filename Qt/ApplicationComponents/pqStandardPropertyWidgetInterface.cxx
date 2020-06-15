/*=========================================================================

   Program: ParaView
   Module: pqStandardPropertyWidgetInterface.cxx

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
#include "pqStandardPropertyWidgetInterface.h"

#include "vtkPVConfig.h"

#include "pqAnimationShortcutDecorator.h"
#include "pqArrayStatusPropertyWidget.h"
#include "pqBackgroundEditorWidget.h"
#include "pqBoxPropertyWidget.h"
#include "pqCTHArraySelectionDecorator.h"
#include "pqCalculatorWidget.h"
#include "pqCameraManipulatorWidget.h"
#include "pqCheckableProperty.h"
#include "pqColorAnnotationsPropertyWidget.h"
#include "pqColorEditorPropertyWidget.h"
#include "pqColorOpacityEditorWidget.h"
#include "pqColorPaletteSelectorWidget.h"
#include "pqColorSelectorPropertyWidget.h"
#include "pqCommandButtonPropertyWidget.h"
#include "pqCompositePropertyWidgetDecorator.h"
#include "pqCylinderPropertyWidget.h"
#include "pqDataAssemblyPropertyWidget.h"
#include "pqDisplayRepresentationWidget.h"
#include "pqDoubleRangeSliderPropertyWidget.h"
#include "pqEnableWidgetDecorator.h"
#include "pqFileNamePropertyWidget.h"
#include "pqFontPropertyWidget.h"
#include "pqGenericPropertyWidgetDecorator.h"
#include "pqGlyphScaleFactorPropertyWidget.h"
#include "pqHandlePropertyWidget.h"
#include "pqImageCompressorWidget.h"
#include "pqImplicitPlanePropertyWidget.h"
#include "pqIndexSelectionWidget.h"
#include "pqInputDataTypeDecorator.h"
#include "pqInputSelectorWidget.h"
#include "pqIntMaskPropertyWidget.h"
#include "pqLightPropertyWidget.h"
#include "pqLinePropertyWidget.h"
#include "pqListPropertyWidget.h"
#include "pqMoleculePropertyWidget.h"
#include "pqMultiComponentsDecorator.h"
#include "pqOMETransferFunctionsPropertyWidget.h"
#include "pqOSPRayHidingDecorator.h"
#include "pqPauseLiveSourcePropertyWidget.h"
#include "pqPropertyCollectionWidget.h"
#include "pqProxyEditorPropertyWidget.h"
#include "pqSeriesEditorPropertyWidget.h"
#include "pqShaderReplacementsSelectorPropertyWidget.h"
#include "pqShowWidgetDecorator.h"
#include "pqSpherePropertyWidget.h"
#include "pqSplinePropertyWidget.h"
#include "pqTextLocationWidget.h"
#include "pqTextureSelectorPropertyWidget.h"
#include "pqTransferFunctionWidgetPropertyWidget.h"
#include "pqViewResolutionPropertyWidget.h"
#include "pqViewTypePropertyWidget.h"
#include "pqYoungsMaterialPropertyWidget.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"

#include <QtDebug>

//-----------------------------------------------------------------------------
pqStandardPropertyWidgetInterface::pqStandardPropertyWidgetInterface(QObject* p)
  : QObject(p)
{
}

//-----------------------------------------------------------------------------
pqStandardPropertyWidgetInterface::~pqStandardPropertyWidgetInterface()
{
}

//-----------------------------------------------------------------------------
pqPropertyWidget* pqStandardPropertyWidgetInterface::createWidgetForProperty(
  vtkSMProxy* smProxy, vtkSMProperty* smProperty, QWidget* parentWidget)
{
  // handle properties that specify custom panel widgets
  const char* custom_widget = smProperty->GetPanelWidget();
  if (!custom_widget)
  {
    return NULL;
  }

  std::string name = custom_widget;

  // *** NOTE: When adding new types, please update the header documentation ***
  if (name == "color_palette_selector")
  {
    return new pqColorPaletteSelectorWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "color_selector")
  {
    bool withPalette = false;
    return new pqColorSelectorPropertyWidget(smProxy, smProperty, withPalette, parentWidget);
  }
  else if (name == "color_selector_with_palette")
  {
    bool withPalette = true;
    return new pqColorSelectorPropertyWidget(smProxy, smProperty, withPalette, parentWidget);
  }
  else if (name == "display_representation_selector")
  {
    return new pqDisplayRepresentationPropertyWidget(smProxy, parentWidget);
  }
  else if (name == "texture_selector")
  {
    return new pqTextureSelectorPropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "shader_replacements_selector")
  {
    return new pqShaderReplacementsSelectorPropertyWidget(smProxy, smProperty);
  }
  else if (name == "calculator")
  {
    return new pqCalculatorWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "command_button")
  {
    return new pqCommandButtonPropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "transfer_function_editor")
  {
    return new pqTransferFunctionWidgetPropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "list")
  {
    return new pqListPropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "double_range")
  {
    return new pqDoubleRangeSliderPropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "image_compressor_config")
  {
    return new pqImageCompressorWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "index_selection")
  {
    return new pqIndexSelectionWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "input_selector")
  {
    return new pqInputSelectorWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "camera_manipulator")
  {
    return new pqCameraManipulatorWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "viewtype_selector")
  {
    return new pqViewTypePropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "glyph_scale_factor")
  {
    return new pqGlyphScaleFactorPropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "proxy_editor")
  {
    return new pqProxyEditorPropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "int_mask")
  {
    return new pqIntMaskPropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "filename_widget")
  {
    return new pqFileNamePropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "view_resolution")
  {
    return new pqViewResolutionPropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "pause_livesource")
  {
    return new pqPauseLiveSourcePropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "data_assembly_editor")
  {
    return new pqDataAssemblyPropertyWidget(smProxy, smProperty, parentWidget);
  }

  // *** NOTE: When adding new types, please update the header documentation ***
  return NULL;
}

//-----------------------------------------------------------------------------
pqPropertyWidget* pqStandardPropertyWidgetInterface::createWidgetForPropertyGroup(
  vtkSMProxy* proxy, vtkSMPropertyGroup* group, QWidget* parentWidget)
{
  QString panelWidget(group->GetPanelWidget());
  // *** NOTE: When adding new types, please update the header documentation ***
  if (panelWidget == "ColorEditor")
  {
    return new pqColorEditorPropertyWidget(proxy, parentWidget);
  }
  else if (panelWidget == "CubeAxes")
  {
    qWarning(
      "`CubeAxes` is no longer supported. Please update your ServerManager XML configuration.");
  }
  else if (panelWidget == "BackgroundEditor")
  {
    return new pqBackgroundEditorWidget(proxy, group, parentWidget, false);
  }
  else if (panelWidget == "EnvironmentalBGEditor")
  {
    return new pqBackgroundEditorWidget(proxy, group, parentWidget, true);
  }
  else if (panelWidget == "ArrayStatus")
  {
    return new pqArrayStatusPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "ColorOpacityEditor")
  {
    return new pqColorOpacityEditorWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "AnnotationsEditor")
  {
    return new pqColorAnnotationsPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "FontEditor")
  {
    return new pqFontPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "SeriesEditor")
  {
    return new pqSeriesEditorPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "MoleculeParameters")
  {
    return new pqMoleculePropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "TextLocationEditor")
  {
    return new pqTextLocationWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractivePlane")
  {
    return new pqImplicitPlanePropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractiveBox")
  {
    return new pqBoxPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractiveHandle")
  {
    return new pqHandlePropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractiveLine")
  {
    return new pqLinePropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractiveSpline")
  {
    return new pqSplinePropertyWidget(proxy, group, pqSplinePropertyWidget::SPLINE, parentWidget);
  }
  else if (panelWidget == "InteractiveSphere")
  {
    return new pqSpherePropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractivePolyLine")
  {
    return new pqSplinePropertyWidget(proxy, group, pqSplinePropertyWidget::POLYLINE, parentWidget);
  }
  else if (panelWidget == "YoungsMaterial")
  {
    return new pqYoungsMaterialPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractiveCylinder")
  {
    return new pqCylinderPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractiveLight")
  {
    return new pqLightPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "OMETransferFunctions")
  {
    return new pqOMETransferFunctionsPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "PropertyCollection")
  {
    return new pqPropertyCollectionWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "DataAssemblyEditor")
  {
    return new pqDataAssemblyPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "CheckableProperty")
  {
    return new pqCheckableProperty(proxy, group, parentWidget);
  }
  // *** NOTE: When adding new types, please update the header documentation ***

  return 0;
}

//-----------------------------------------------------------------------------
pqPropertyWidgetDecorator* pqStandardPropertyWidgetInterface::createWidgetDecorator(
  const QString& type, vtkPVXMLElement* config, pqPropertyWidget* widget)
{
  // *** NOTE: When adding new types, please update the header documentation ***
  if (type == "CTHArraySelectionDecorator")
  {
    return new pqCTHArraySelectionDecorator(config, widget);
  }
  if (type == "InputDataTypeDecorator")
  {
    return new pqInputDataTypeDecorator(config, widget);
  }
  if (type == "EnableWidgetDecorator")
  {
    return new pqEnableWidgetDecorator(config, widget);
  }
  if (type == "ShowWidgetDecorator")
  {
    return new pqShowWidgetDecorator(config, widget);
  }
  if (type == "GenericDecorator")
  {
    return new pqGenericPropertyWidgetDecorator(config, widget);
  }
  if (type == "OSPRayHidingDecorator")
  {
    return new pqOSPRayHidingDecorator(config, widget);
  }
  if (type == "MultiComponentsDecorator")
  {
    return new pqMultiComponentsDecorator(config, widget);
  }
  if (type == "CompositeDecorator")
  {
    return new pqCompositePropertyWidgetDecorator(config, widget);
  }

  // *** NOTE: When adding new types, please update the header documentation ***
  return NULL;
}

//-----------------------------------------------------------------------------
void pqStandardPropertyWidgetInterface::createDefaultWidgetDecorators(pqPropertyWidget* widget)
{
  // *** NOTE: When adding new default decorators, please update the header documentation ***
  if (pqAnimationShortcutDecorator::accept(widget))
  {
    new pqAnimationShortcutDecorator(widget);
  }
  // *** NOTE: When adding new default decorators, please update the header documentation ***
}
