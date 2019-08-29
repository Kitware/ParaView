/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef pqColorOpacityEditorWidget_h
#define pqColorOpacityEditorWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include "pqSMProxy.h"
#include <QList>
#include <QVariant>

class vtkPiecewiseFunction;
class vtkSMPropertyGroup;
class pqColorMapModel;

/**
* pqColorOpacityEditorWidget provides an in-line editor widget for editing the
* color and opacity transfer functions. The property group is expected to have
* properties with the following functions. If any of the optional properties
* are missing, then the corresponding widgets are hidden.
* \li "XRGBPoints"            :- property with (x,r,g,b) tuples that is
*                                controlled by a color-transfer function editor
*                                (pqTransferFunctionWidget).
* \li "ScalarOpacityFunction" :- (optional) proxy-property referring to a proxy with
*                                "Points" property with (x,a,m,s) tuples that
*                                is controlled by an opacity-transfer function
*                                editor (pqTransferFunctionWidget).
* \li "EnableOpacityMapping"  :- (optional) property used to enable
*                                opacity mapping for surfaces. Controlled by a
*                                checkbox in the Widget.
* \li "UseLogScale"           :- (optional) property used to enable/disable log mapping
*                                for colors.
* \li "UseLogScaleOpacity"    :- (optional) property used to enable/disable log mapping
*                                for opacity.
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
  Q_PROPERTY(pqSMProxy scalarOpacityFunctionProxy READ scalarOpacityFunctionProxy WRITE
      setScalarOpacityFunctionProxy)
  typedef pqPropertyWidget Superclass;

public:
  pqColorOpacityEditorWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = 0);
  ~pqColorOpacityEditorWidget() override;

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

public slots:
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
  void choosePreset(const char* presetName = NULL);

  /**
  * save current transfer function as preset.
  */
  void saveAsPreset();

signals:
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

protected slots:
  /**
  * slots called when the current point changes on the two internal
  * pqTransferFunctionWidget widgets.
  */
  void opacityCurrentChanged(vtkIdType);
  void colorCurrentChanged(vtkIdType);

  /**
  * updates the panel to show/hide advanced settings
  */
  void updatePanel();

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
  * Ensures that the color-swatches for indexedColors are shown only when this
  * is set to true.
  */
  void updateIndexedLookupState();

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
  * Called when the showDataHistogram checkbox is clicked by the user. We then add
  * extra logic to compute and update the histogram and enable to update histogram button.
  * This uses the histogram table cache if it is up to date.
  * This can be called manually to show/hide the histogram
  */
  void showDataHistogramClicked(bool show = true);

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
   */
  void setHistogramOutdated();

  /**
   * Slot called by a timer which is triggered in showDataHistogramClicked
   * This method actually setup the histogram
   * but should not be used directly
   */
  void realShowDataHistogram();

protected:
  /**
   * Validate and adjust the current range before converting to a log range.
   */
  void prepareRangeForLogScaling();

  /**
   * Initialize the opacity editor.
   */
  void initializeOpacityEditor(vtkPiecewiseFunction* pwf);

private:
  Q_DISABLE_COPY(pqColorOpacityEditorWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
