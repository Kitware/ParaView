// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqStandardPropertyWidgetInterface_h
#define pqStandardPropertyWidgetInterface_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetInterface.h"

/**
 * pqStandardPropertyWidgetInterface provides a concrete implementation of
 * pqPropertyWidget used by ParaView application. It adds logic to create some
 * of the custom widgets and decorators used by ParaView's Properties Panel.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqStandardPropertyWidgetInterface
  : public QObject
  , public pqPropertyWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(pqPropertyWidgetInterface)
public:
  pqStandardPropertyWidgetInterface(QObject* p = nullptr);
  ~pqStandardPropertyWidgetInterface() override;

  /**
   * Given a proxy and its property, create a widget for the same, if possible.
   * For unsupported/unknown proxies/properties, implementations should simply
   * return nullptr without raising any errors (or messages).
   * Supported types are:
   * \li \c calculator : pqCalculatorWidget
   * \li \c camera_manipulator : pqCameraManipulatorWidget
   * \li \c color_palette_selector : pqColorPaletteSelectorWidget
   * \li \c color_selector : pqColorSelectorPropertyWidget
   * \li \c color_selector_with_palette : pqColorSelectorPropertyWidget with palette menu
   * \li \c command_button : pqCommandButtonPropertyWidget
   * \li \c display_representation_selector : pqDisplayRepresentationPropertyWidget
   * \li \c double_range : pqDoubleRangeSliderPropertyWidget
   * \li \c filename_widget: pqFileNamePropertyWidget
   * \li \c glyph_scale_factor: pqGlyphScaleFactorPropertyWidget
   * \li \c image_compressor_config : pqImageCompressorWidget
   * \li \c index_selection : pqIndexSelectionWidget
   * \li \c input_selector : pqInputSelectorWidget
   * \li \c int_mask: pqIntMaskPropertyWidget
   * \li \c list : pqListPropertyWidget
   * \li \c proxy_editor: pqProxyEditorPropertyWidget
   * \li \c texture_selector : pqTextureSelectorPropertyWidget
   * \li \c transfer_function_editor : pqTransferFunctionWidgetPropertyWidget
   * \li \c viewtype_selector: pqViewTypePropertyWidget
   * \li \c language_selector: pqLanguageChooserWidget
   * \li \c view_resolution: pqViewResolutionPropertyWidget
   * \li \c pause_livesource: pqPauseLiveSourcePropertyWidget
   * \li \c data_assembly_editor: pqDataAssemblyPropertyWidget
   * \li \c selection_query : pqSelectionQueryPropertyWidget
   * \li \c file_list : pqFileListPropertyWidget
   * \li \c xy_chart_bounds : pqXYChartViewBoundsPropertyWidget
   */
  pqPropertyWidget* createWidgetForProperty(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parentWidget) override;

  /**
   * Given a proxy and its property group, create a widget for the same, of possible.
   * For unsupported/unknown proxies/property-groups, implementations should simply
   * return nullptr without raising any errors (or messages).
   * Supported types are:
   * \li \c AnnotationsEditor : pqColorAnnotationsPropertyWidget
   * \li \c ArrayStatus : pqArrayStatusPropertyWidget
   * \li \c BackgroundEditor : pqBackgroundPropertyWidget
   * \li \c ColorEditor : pqColorEditorPropertyWidget
   * \li \c ColorOpacityEditor : pqColorOpacityEditorWidget
   * \li \c FontEditor : pqFontPropertyWidget
   * \li \c SeriesEditor : pqSeriesEditorPropertyWidget
   * \li \c InteractivePlane : pqDisplaySizedImplicitPlanePropertyWidget
   * \li \c InteractivePlane2 : pqImplicitPlanePropertyWidget
   * \li \c InteractiveBox: pqBoxPropertyWidget
   * \li \c InteractiveHandle: pqHandlePropertyWidget
   * \li \c InteractiveLine: pqLinePropertyWidget
   * \li \c InteractiveSpline: pqSplinePropertyWidget
   * \li \c InteractiveSphere: pqSpherePropertyWidget
   * \li \c InteractivePolyLine: pqSplinePropertyWidget (with mode==POLYLINE)
   * \li \c InteractiveCylinder: pqCylinderPropertyWidget
   * \li \c YoungsMaterial: pqYoungsMaterialPropertyWidget
   * \li \c OMETransferFunctions : pqOMETransferFunctionsPropertyWidget
   * \li \c PropertyCollection : pqPropertyCollectionWidget
   * \li \c DataAssemblyEditor: pqDataAssemblyPropertyWidget
   * \li \c CheckableProperty : pqCheckableProperty
   * \li \c EqualizerPropertyWidget : pqEqualizerPropertyWidget
   * \li \c MetaDataPropertyWidget : pqMetaDataPropertyWidget
   */
  pqPropertyWidget* createWidgetForPropertyGroup(
    vtkSMProxy* proxy, vtkSMPropertyGroup* group, QWidget* parentWidget) override;

  /**
   * Given the type of the decorator and the pqPropertyWidget that needs to be
   * decorated, create the pqPropertyWidgetDecorator instance, if possible.
   * For unsupported/unknown, implementations should simply return nullptr without
   * raising any errors (or messages).
   * Supported types are:
   * \li \c CTHArraySelectionDecorator : pqCTHArraySelectionDecorator
   * \li \c EnableWidgetDecorator : pqEnableWidgetDecorator
   * \li \c ShowWidgetDecorator : pqShowWidgetDecorator
   * \li \c InputDataTypeDecorator : pqInputDataTypeDecorator
   * \li \c GenericDecorator: pqGenericPropertyWidgetDecorator
   * \li \c OSPRayHidingDecorator: pqOSPRayHidingDecorator
   * \li \c MultiComponentsDecorator: pqMultiComponentsDecorator
   * \li \c CompositeDecorator: pqCompositePropertyWidgetDecorator
   * \li \c SessionTypeDecorator: pqSessionTypeDecorator
   */
  pqPropertyWidgetDecorator* createWidgetDecorator(
    const QString& type, vtkPVXMLElement* config, pqPropertyWidget* widget) override;

  /**
   * Create all default decorators for a specific widget.
   * Created decorators are:
   * \li \c AnimationShortcutDecorator : pqAnimationShortcutDecorator
   */
  void createDefaultWidgetDecorators(pqPropertyWidget* widget) override;
};

#endif // pqStandardPropertyWidgetInterface_h
