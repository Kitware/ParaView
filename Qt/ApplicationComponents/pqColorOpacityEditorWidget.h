// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqColorOpacityEditorWidget_h
#define pqColorOpacityEditorWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include "pqSMProxy.h"
#include <QList>
#include <QVariant>

class pqColorMapModel;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkSMPropertyGroup;
class vtkPVTransferFunction2D;

/**
 * pqColorOpacityEditorWidget provides an in-line editor widget for editing the
 * color and opacity transfer functions. The property group is expected to have
 * properties with the following functions. If any of the optional properties
 * are missing, then the corresponding widgets are hidden.
 * \li "XRGBPoints"                              :- property with (x,r,g,b) tuples that is
 *                                                  controlled by a color-transfer function
 *                                                  editor (pqTransferFunctionWidget).
 * \li "ScalarOpacityFunction"                   :- (optional) proxy-property referring to a proxy
 *                                                  with "Points" property with (x,a,m,s) tuples
 *                                                  that is controlled by an opacity-transfer
 *                                                  function editor (pqTransferFunctionWidget).
 * \li "EnableOpacityMapping"                    :- (optional) property used to enable
 *                                                  opacity mapping for surfaces. Controlled by a
 *                                                  checkbox in the Widget.
 * \li "UseLogScale"                             :- (optional) property used to enable/disable log
 *                                                  mapping for colors.
 * \li "UseLogScaleOpacity"                      :- (optional) property used to enable/disable log
 *                                                  mapping for opacity.
 * \li "UseOpacityControlPointsFreehandDrawing"  :- (optional) property used to enable/disable
 *                                                  freehand drawing for positioning control points
 * \li "Transfer2DBoxes"                         :- (optional) property with (x0, y0, width, height,
 *                                                  r, g, b, a) tuples that is controlled by the 2D
 *                                                  transfer function editor
 *                                                  (pqTransferFunction2DWidget).
 * Caveats:
 * \li Opacity editor:- pqColorOpacityEditorWidget shows an opacity editor widget.
 * Typically, opacity function is optional and used only when
 * "EnableOpacityMapping" is property is ON. However, in cases of Volume
 * rendering, the EnableOpacityMapping has no effect.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqColorOpacityEditorWidget : public pqPropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> xrgbPoints READ xrgbPoints WRITE setXrgbPoints)
  Q_PROPERTY(QList<QVariant> xvmsPoints READ xvmsPoints WRITE setXvmsPoints)
  Q_PROPERTY(bool showDataHistogram READ showDataHistogram WRITE setShowDataHistogram)
  Q_PROPERTY(bool automaticDataHistogramComputation READ automaticDataHistogramComputation WRITE
      setAutomaticDataHistogramComputation)
  Q_PROPERTY(
    int dataHistogramNumberOfBins READ dataHistogramNumberOfBins WRITE setDataHistogramNumberOfBins)
  Q_PROPERTY(bool useLogScale READ useLogScale WRITE setUseLogScale)
  Q_PROPERTY(bool useLogScaleOpacity READ useLogScaleOpacity WRITE setUseLogScaleOpacity)
  Q_PROPERTY(bool useOpacityControlPointsFreehandDrawing READ useOpacityControlPointsFreehandDrawing
      WRITE setUseOpacityControlPointsFreehandDrawing)
  Q_PROPERTY(pqSMProxy scalarOpacityFunctionProxy READ scalarOpacityFunctionProxy WRITE
      setScalarOpacityFunctionProxy)
  Q_PROPERTY(
    pqSMProxy transferFunction2DProxy READ transferFunction2DProxy WRITE setTransferFunction2DProxy)
  Q_PROPERTY(QList<QVariant> transfer2DBoxes READ transfer2DBoxes WRITE setTransfer2DBoxes)

  typedef pqPropertyWidget Superclass;

public:
  pqColorOpacityEditorWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqColorOpacityEditorWidget() override;

  /**
   * updates the panel to show/hide advanced settings
   */
  void updateWidget(bool showing_advanced_properties) override;

  /**
   * Returns the current list of control points for the color transfer
   * function. This a list of 4-tuples.
   */
  QList<QVariant> xrgbPoints() const;

  /**
   * Returns the current list of control points for the opacity
   * function. This a list of 4-tuples.
   */
  QList<QVariant> xvmsPoints() const;

  /**
   * Returns the value for use-log-scale.
   */
  bool useLogScale() const;

  /**
   * Returns the value for use-log-scale.
   */
  bool useLogScaleOpacity() const;

  /**
   * Returns the value for the use of freehand drawing when positioning opacity control points.
   */
  bool useOpacityControlPointsFreehandDrawing() const;

  /**
   * Returns the value for showDataHistogram
   */
  bool showDataHistogram() const;

  /**
   * Returns the value for automaticDataHistogramComputation
   */
  bool automaticDataHistogramComputation() const;

  /**
   * Returns the value for dataHistogramNumberOfBins
   */
  int dataHistogramNumberOfBins() const;

  /**
   * Returns the scalar opacity function (i.e. PiecewiseFunction) proxy
   * used, if any.
   */
  pqSMProxy scalarOpacityFunctionProxy() const;

  /**
   * Returns the 2D transfer function (i.e. vtkImageData) proxy used, if any.
   */
  pqSMProxy transferFunction2DProxy() const;

  /**
   * Returns the value for Using2DTransferFunction internal server-manager property.
   */
  bool using2DTransferFunction() const;

  /**
   * Returns the current list of boxes for the 2D transfer
   * function. This a list of 8-tuples - [x0, y0, width, height, r, g, b, opacity].
   */
  QList<QVariant> transfer2DBoxes() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Sets the xvmsPoints that control the opacity transfer function.
   */
  void setXvmsPoints(const QList<QVariant>&);

  /**
   * Sets the xrgbPoints that control the color transfer function.
   */
  void setXrgbPoints(const QList<QVariant>&);

  /**
   * Set whether to use-log scale.
   */
  void setUseLogScale(bool value);

  /**
   * Set whether to use-log scale.
   */
  void setUseLogScaleOpacity(bool value);

  /**
   * Set whether to use freehand drawing for positioning opacity control points.
   */
  void setUseOpacityControlPointsFreehandDrawing(bool value);

  /**
   * Set whether to show data histogram.
   */
  void setShowDataHistogram(bool value);

  /**
   * Set whether to automatically compute data histogram.
   */
  void setAutomaticDataHistogramComputation(bool value);

  /**
   * Set the number of bins to use for the data histogram.
   */
  void setDataHistogramNumberOfBins(int value);

  /**
   * Set the scalar opacity function (or PiecewiseFunction) proxy to use.
   */
  void setScalarOpacityFunctionProxy(pqSMProxy sofProxy);

  /**
   * Set the 2D transfer function (or vtkImageData) proxy to use.
   */
  void setTransferFunction2DProxy(pqSMProxy t2dProxy);

  /**
   * Reset the transfer function ranges to active data source.
   */
  void resetRangeToData();

  /**
   * Reset the transfer function ranges to custom values.
   */
  void resetRangeToCustom();

  /**
   * Reset the transfer function ranges to temporal range for active data
   * source.
   */
  void resetRangeToDataOverTime();

  /**
   * Reset the transfer function ranges to visible range for active data
   * source.
   */
  void resetRangeToVisibleData();

  /**
   * Inverts the transfer functions.
   */
  void invertTransferFunctions();

  /**
   * pick a preset.
   */
  void choosePreset(const char* presetName = nullptr);

  /**
   * save current transfer function as preset.
   */
  void saveAsPreset();

  /**
   * Compute the data histogram 1D or 2D. Depends on the "Using2DTransferFunction" internal
   * property.
   */
  void computeDataHistogram();

  void onRangeHandlesRangeChanged(double rangeMin, double rangeMax);

  /**
   * Resets the color map combo box by setting its index
   * to -1 which sets its text to an empty string
   */
  void resetColorMapComboBox();

  /**
   * Sets the box items that control the 2D transfer function.
   */
  void setTransfer2DBoxes(const QList<QVariant>&);

  /**
   * Choose a color and alpha for the currently active 2D transfer function box.
   */
  void chooseBoxColorAlpha();

Q_SIGNALS:
  /**
   * Signal fired when the xrgbPoints change.
   */
  void xrgbPointsChanged();

  /**
   * Signal fired when the xvmsPoints change.
   */
  void xvmsPointsChanged();

  /**
   * Signal fired when useLogScale changes.
   */
  void useLogScaleChanged();

  /**
   * Signal fired when useLogScaleOpacity changes.
   */
  void useLogScaleOpacityChanged();

  /**
   * Signal fired when useOpacityControlPointsFreehandDrawing changes.
   */
  void useOpacityControlPointsFreehandDrawingChanged();

  /**
   * Signal fired when showDataHistogram changes.
   */
  void showDataHistogramChanged();

  /**
   * Signal fired when automaticDataHistogramComputation changes.
   */
  void automaticDataHistogramComputationChanged();

  /**
   * Signal fired when dataHistogramNumberOfBins changes.
   */
  void dataHistogramNumberOfBinsEdited();

  /**
   * This signal is never really fired since this
   * widget doesn't have any UI to allow users to changes the
   * ScalarOpacityFunction proxy used.
   */
  void scalarOpacityFunctionProxyChanged();

  /**
   * Signal fired when the transfer function 2D proxy is changed.
   */
  void transferFunction2DProxyChanged();

  /**
   * Signal fired when the transfer function 2d boxes change.
   */
  void transfer2DBoxesChanged();

protected Q_SLOTS:
  /**
   * slots called when the current point changes on the two internal
   * pqTransferFunctionWidget widgets.
   */
  void opacityCurrentChanged(vtkIdType);
  void colorCurrentChanged(vtkIdType);

  /**
   * updates the text shown in the "current data" input.
   */
  void updateCurrentData();

  /**
   * update the transfer function with the text in CurrentDataValue text field.
   */
  void currentDataEdited();

  /**
   * called when a preset is applied.
   */
  void presetApplied();

  /**
   * Updates the default presets combo box when the list changes
   */
  void updateDefaultPresetsList();

  /**
   * called when "MultiComponentsMappingChanged" checkbox is modified.
   */
  void multiComponentsMappingChanged(vtkObject*, unsigned long, void*, void*);

  /**
   * called when the use-log-scale checkbox is clicked by the user. We then add
   * extra logic to valid ranges convert the color map to log/linear space.
   */
  void useLogScaleClicked(bool);

  /**
   * called when the use-log-scale checkbox is clicked by the user. We then add
   * extra logic to valid ranges convert the color map to log/linear space.
   */
  void useLogScaleOpacityClicked(bool);

  /**
   * Called when UseOpacityControlPointsFreehandDrawing checkbox
   * is clicked by the user. We transfer the call to the pqTransferFunctionEditor.
   */
  void useOpacityControlPointsFreehandDrawingClicked(bool);

  /**
   * Called when the showDataHistogram checkbox is clicked by the user. We then add
   * extra logic to compute and update the histogram and enable to update histogram button.
   * This uses the histogram table cache if it is up to date.
   * This can be called manually to show/hide the histogram
   */
  void showDataHistogramClicked(bool show = true);

  /**
   * Called when the use2DTransferFunction checkbox is clicked by the user.
   */
  void show2DHistogram(bool show = true);

  /**
   * Called when the automaticDataHistogramComputation checkbox is clicked by the user.
   * We then add extra logic to show the histogram and update the UI.
   */
  void automaticDataHistogramComputationClicked(bool val);

  /**
   * Called when the  dataHistogramNumberOfBins is changed by the user.
   * We then add extra logic to set the histogram outdated
   */
  void dataHistogramNumberOfBinsEdited(int val);

  /**
   * Update the enable state of different data histogram related widgets
   */
  void updateDataHistogramEnableState();

  /**
   * called when the active representation or view changes.  We then change the
   * enabled/disabled state of the buttons. Some actions require a valid
   * representation or view, so disable them if there isn't one.
   * We also update the opacity editor.
   */
  void representationOrViewChanged();

  /**
   * Called when the histogram should be considered outdated, which includes :
   * When the range of the transfer function is changed, either from a custom range or from an
   * updated range
   * When the vector mode or vector component is changed
   * When the visibility of a consumer is changed
   * When the number of bins is changed
   * When the 2D transfer function arrays are changed
   */
  void setHistogramOutdated();

  /**
   * Slot called by a timer which is triggered in showDataHistogramClicked
   * This method actually setup the histogram
   * but should not be used directly
   */
  void realShowDataHistogram();

  /**
   * Slot called by a timer which is triggered in show2DHistogram
   * This method actually sets up the 2D histogram
   */
  void realShow2DHistogram();

  /**
   * Slot to update active representation when the 2D transfer function has changed with the
   * internal pqTransferFunction2DWidget class.
   */
  void transfer2DChanged();

  /**
   * Slot to update active representation when the piecewise function for opacity is modified.
   * For ex, editing the midpoint, sharpness around a piecewise control point modifies the function.
   */
  void opacityFunctionModified();

  /**
   * Slot to update the 2D transfer function proxy when the Y axis scalar array is changed.
   */
  void updateTransferFunction2DProxy();

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Validate and adjust the current range before converting to a log range.
   */
  void prepareRangeForLogScaling();

  /**
   * Initialize the opacity editor.
   */
  void initializeOpacityEditor(vtkPiecewiseFunction* pwf);

  /**
   * Initialize the 2D transfer function editor
   */
  void initializeTransfer2DEditor(vtkPVTransferFunction2D* tf2d);

  /**
   * When representation has changed, gotta reconnect some internal observers.
   */
  void observeRepresentationModified(vtkSMProxy* reprProxy, vtkPiecewiseFunction* pwf);

private:
  Q_DISABLE_COPY(pqColorOpacityEditorWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
