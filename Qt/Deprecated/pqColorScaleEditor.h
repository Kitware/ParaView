/*=========================================================================

   Program: ParaView
   Module:    pqColorScaleEditor.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

/// \file pqColorScaleEditor.h
/// \date 2/14/2007

#ifndef _pqColorScaleEditor_h
#define _pqColorScaleEditor_h


#include "pqDeprecatedModule.h"
#include <QDialog>

class pqColorScaleEditorForm;
class pqDataRepresentation;
class pqScalarBarRepresentation;
class pqScalarOpacityFunction;
class pqScalarsToColors;
class QHideEvent;
class QShowEvent;
class QString;
class pqTransferFunctionChartViewWidget;
class vtkAbstractArray;
class vtkPlot;
class vtkObject;
class vtkColorTransferFunction;
class vtkPiecewiseFunction;

/**\brief A dialog for editing color and/or opacity maps.
  *
  * This function ties the user interface elements of the color/transfer-function editor to server proxies
  * that map from point/cell-scalars to color and/or opacity values.
  *
  * The \a ColorMapViewer, \a OpacityFunctionViewer, and entries of the internal \a Form member
  * are GUI elements with proxies in \a ColorMap, \a OpacityFunction, and \a Legend.
  * \a Display is a proxy used to obtain information on the scalar range (for interval data) or
  * unique values (for categorical data) of the currently-active representation.
  * If a small set of unique values exist for the currently-selected field component,
  * it is cached in \a UniqueValues and added to the Annotations table in the UI when the user
  * clicks on "Add ## Values" button in the "Annotations" tab of the dialog.
  *
  * This class does not strictly follow the Principle of Minimum Astonishment;
  * while its "Apply" and "Close" buttons are distinct, the original state is not
  * restored if "Close" is pressed after edits without clicking "Apply".
  * Instead, changes are always applied when closing the dialog.
  * Be aware that this means there is no model separate from the state stored in the GUI.
  *
  * The purpose behind this inconsistency is to reduce the number of renders while editing the
  * colormap or transfer function.
  * At a minimum, the "Apply" button should probably be placed next to the "Render immediately"
  * checkbox and disabled when "Render immediately" is checked.
  */
class PQDEPRECATED_EXPORT pqColorScaleEditor : public QDialog
{
  Q_OBJECT

public:
  pqColorScaleEditor(QWidget *parent=0);
  virtual ~pqColorScaleEditor();

public slots:
  void setRepresentation(pqDataRepresentation *display);

protected:
//  virtual void showEvent(QShowEvent *e);
//  virtual void hideEvent(QHideEvent *e);

  /// Set/get a list of unique values associated with the current array that the user may draw from.
  vtkAbstractArray* getActiveUniqueValues() { return this->ActiveUniqueValues; }
  virtual void setActiveUniqueValues( vtkAbstractArray* );

  /// A helper to capture user-directed reorderings (via drag and drop) of items in the annotation list.
  virtual bool eventFilter( QObject* obj, QEvent* event );

private slots:
  void applyPreset();
  void updateColors();
  void updateOpacity();

  /// \name Callbacks invoked upon property changes on proxy objects
  //@{
  void handleEnableOpacityMappingChanged();
  void handleOpacityPointsChanged();
  void handleColorPointsChanged();
  void handleInterpretationChanged();
  void handleAnnotationsChanged();
  //@}

  void setScalarFromText();
  void setOpacityFromText();
  void setSingleOpacityFromText();
  void setOpacityScalarFromText();
  void setOpacityControlsVisibility(bool visible);
  void setEnableOpacityMapping(int enable);
  void setInterpretation( int buttonId );
  void updateDisplay();

  void setColorSpace(int index);
  void internalSetColorSpace(int index,
    vtkColorTransferFunction*);

  void setNanColor(const QColor &color);
  void setNanColor2(const QColor &color);
  void setScalarColor(const QColor &color);
  void setScalarButtonColor(const QColor &color);

  void savePreset();
  void loadPreset();

  /// \name Range and scale (linear/log) methods
  //@{
  void setLogScale(bool on);

  void setAutoRescale(bool on);
  void rescaleToNewRange();
  void rescaleToDataRange();
  void rescaleToDataRangeOverTime();
  void rescaleToSimpleRange();

  void setScalarRange(double min, double max);
  //@}

  /// \name Interval/ratio colormap discretization options
  //@{
  void setUseDiscreteColors(bool on);
  void setSizeFromText();
  void setSizeFromSlider(int tableSize);
  void setTableSize(int tableSize);
  //@}

  /// \name Annotation methods
  //@{
  void resetAnnotations();
  void removeAnnotation();
  void addActiveValues();
  void addAnnotationEntry();
  void annotationsChanged();
  void resetAnnotationSort();
  void updateAnnotationColors();
  void annotationSelectionChanged();
  void editAnnotationColor(const QColor&);
  //@}

  /// \name Color Legend Methods
  //@{
  void checkForLegend();
  void setLegendVisibility(bool visible);
  void updateLegendVisibility(bool visible);
  void setLegendName(const QString &text);
  void setLegendComponent(const QString &text);
  void setLegendTitle(const QString &name, const QString &component);
  void updateLegendTitle();
  void updateLabelFormatControls();
  //@}

  /// \name Cleanup Methods
  //@{
  void cleanupDisplay();
  void cleanupLegend();
  //@}

  /// MakeDefaultButton callback.
  void makeDefault();

  // Transfer function chart view slots
  void onColorPlotAdded(vtkPlot*);
  void onOpacityPlotAdded(vtkPlot*);
  void updateCurrentColorPoint();
  void updateCurrentOpacityPoint();
  void enableColorPointControls();
  void enableOpacityPointControls();
  void renderViewOptionally();
  void saveOptionalUserSettings();
  void restoreOptionalUserSettings();

  // Advanced/Simple management
  void enableAdvancedPanel(bool);


private:
  void loadBuiltinColorPresets();
  void loadAnnotations();
  void loadColorPoints();
  void loadOpacityPoints();
  void initColorScale();
  void enablePointControls();
  void updatePointValues();
  void enableRescaleControls(bool enable);
  void enableResolutionControls(bool enable);
  void updateScalarRange(double min, double max);
  void setLegend(pqScalarBarRepresentation *legend);
  void enableLegendControls(bool enable);

  // Initialize the transfer function view widgets
  void initTransferFunctionView();
  vtkColorTransferFunction* currentColorFunction();
  vtkPiecewiseFunction* currentOpacityFunction();
  void updateColorFunctionVisibility();
  void updateOpacityFunctionVisibility();
  bool internalScalarRange(double* range);
  void renderTransferFunctionViews();
  void unsetCurrentPoints();
  void pushColors();
  void pushOpacity();
  void pushAnnotations();

private:
  pqColorScaleEditorForm *Form;
  pqTransferFunctionChartViewWidget* ColorMapViewer;
  pqTransferFunctionChartViewWidget* OpacityFunctionViewer;
  pqDataRepresentation *Display;
  pqScalarsToColors *ColorMap;
  pqScalarOpacityFunction *OpacityFunction;
  pqScalarBarRepresentation *Legend;
  bool UseEnableOpacityCheckBox;
  vtkAbstractArray* ActiveUniqueValues;
};

#endif
