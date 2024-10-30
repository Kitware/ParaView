// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqStandardPropertyWidgetInterface.h"

#include "pqAnglePropertyWidget.h"
#include "pqAnimationShortcutDecorator.h"
#include "pqAnnulusPropertyWidget.h"
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
#include "pqConePropertyWidget.h"
#include "pqCoordinateFramePropertyWidget.h"
#include "pqCylinderPropertyWidget.h"
#include "pqDataAssemblyPropertyWidget.h"
#include "pqDisplayRepresentationWidget.h"
#include "pqDisplaySizedImplicitPlanePropertyWidget.h"
#include "pqDoubleRangeSliderPropertyWidget.h"
#include "pqEnableWidgetDecorator.h"
#include "pqEqualizerPropertyWidget.h"
#include "pqFileListPropertyWidget.h"
#include "pqFileNamePropertyWidget.h"
#include "pqFontPropertyWidget.h"
#include "pqFrustumPropertyWidget.h"
#include "pqGenericPropertyWidgetDecorator.h"
#include "pqGlyphScaleFactorPropertyWidget.h"
#include "pqHandlePropertyWidget.h"
#include "pqImageCompressorWidget.h"
#include "pqImplicitPlanePropertyWidget.h"
#include "pqIndexSelectionWidget.h"
#include "pqInputDataTypeDecorator.h"
#include "pqInputSelectorWidget.h"
#include "pqIntMaskPropertyWidget.h"
#include "pqLanguageChooserWidget.h"
#include "pqLightPropertyWidget.h"
#include "pqLinePropertyWidget.h"
#include "pqListPropertyWidget.h"
#include "pqMetaDataPropertyWidget.h"
#include "pqMoleculePropertyWidget.h"
#include "pqMultiBlockPropertiesEditorWidget.h"
#include "pqMultiComponentsDecorator.h"
#include "pqOMETransferFunctionsPropertyWidget.h"
#include "pqOSPRayHidingDecorator.h"
#include "pqPauseLiveSourcePropertyWidget.h"
#include "pqPropertyCollectionWidget.h"
#include "pqProxyEditorPropertyWidget.h"
#include "pqReaderSelectionPropertyWidget.h"
#include "pqSelectionListPropertyWidget.h"
#include "pqSelectionQueryPropertyWidget.h"
#include "pqSeriesEditorPropertyWidget.h"
#include "pqSessionTypeDecorator.h"
#include "pqShaderReplacementsSelectorPropertyWidget.h"
#include "pqShowWidgetDecorator.h"
#include "pqSpherePropertyWidget.h"
#include "pqSplinePropertyWidget.h"
#include "pqTextLocationWidget.h"
#include "pqTextureSelectorPropertyWidget.h"
#include "pqTransferFunctionWidgetPropertyWidget.h"
#include "pqViewResolutionPropertyWidget.h"
#include "pqViewTypePropertyWidget.h"
#include "pqXYChartViewBoundsPropertyWidget.h"
#include "pqYoungsMaterialPropertyWidget.h"

#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"

#include <QtDebug>

//-----------------------------------------------------------------------------
pqStandardPropertyWidgetInterface::pqStandardPropertyWidgetInterface(QObject* p)
  : QObject(p)
{
}

//-----------------------------------------------------------------------------
pqStandardPropertyWidgetInterface::~pqStandardPropertyWidgetInterface() = default;

//-----------------------------------------------------------------------------
pqPropertyWidget* pqStandardPropertyWidgetInterface::createWidgetForProperty(
  vtkSMProxy* smProxy, vtkSMProperty* smProperty, QWidget* parentWidget)
{
  // handle properties that specify custom panel widgets
  const char* custom_widget = smProperty->GetPanelWidget();
  if (custom_widget == nullptr)
  {
    if (smProperty->FindDomain<vtkSMCompositeTreeDomain>() != nullptr)
    {
      return new pqDataAssemblyPropertyWidget(smProxy, smProperty, parentWidget);
    }
    return nullptr;
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
  else if (name == "language_selector")
  {
    return new pqLanguageChooserWidget(smProxy, smProperty, parentWidget);
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
  else if (name == "selection_query")
  {
    return new pqSelectionQueryPropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "file_list")
  {
    return new pqFileListPropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "reader_selector")
  {
    return new pqReaderSelectionPropertyWidget(smProxy, smProperty, parentWidget);
  }
  else if (name == "xy_chart_bounds")
  {
    return new pqXYChartViewBoundsPropertyWidget(smProxy, smProperty, parentWidget);
  }

  // *** NOTE: When adding new types, please update the header documentation ***
  return nullptr;
}

//-----------------------------------------------------------------------------
pqPropertyWidget* pqStandardPropertyWidgetInterface::createWidgetForPropertyGroup(
  vtkSMProxy* proxy, vtkSMPropertyGroup* group, QWidget* parentWidget)
{
  const QString panelWidget(group->GetPanelWidget());
  // *** NOTE: When adding new types, please update the header documentation ***
  if (panelWidget == "ColorEditor")
  {
    return new pqColorEditorPropertyWidget(proxy, parentWidget, 0 /*Representation*/);
  }
  else if (panelWidget == "BlockColorEditor")
  {
    return new pqColorEditorPropertyWidget(proxy, parentWidget, 1 /*Blocks*/);
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
  else if (panelWidget == "InteractiveAngle")
  {
    return new pqAnglePropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractivePlane")
  {
    return new pqDisplaySizedImplicitPlanePropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractivePlane2")
  {
    return new pqImplicitPlanePropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractiveFrame")
  {
    return new pqCoordinateFramePropertyWidget(proxy, group, parentWidget);
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
  else if (panelWidget == "InteractiveAnnulus")
  {
    return new pqAnnulusPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractiveCone")
  {
    return new pqConePropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractiveCylinder")
  {
    return new pqCylinderPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "InteractiveFrustum")
  {
    return new pqFrustumPropertyWidget(proxy, group, parentWidget);
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
  else if (panelWidget == "EqualizerPropertyWidget")
  {
    return new pqEqualizerPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "MetaDataPropertyWidget")
  {
    return new pqMetaDataPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "SelectionList")
  {
    return new pqSelectionListPropertyWidget(proxy, group, parentWidget);
  }
  else if (panelWidget == "BlockPropertiesEditor")
  {
    return new pqMultiBlockPropertiesEditorWidget(proxy, group, parentWidget);
  }

  // *** NOTE: When adding new types, please update the header documentation ***

  return nullptr;
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
  if (type == "SessionTypeDecorator")
  {
    return new pqSessionTypeDecorator(config, widget);
  }

  // *** NOTE: When adding new types, please update the header documentation ***
  return nullptr;
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
