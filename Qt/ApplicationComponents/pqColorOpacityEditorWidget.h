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
#ifndef __pqColorOpacityEditorWidget_h
#define __pqColorOpacityEditorWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include "vtkType.h"
#include <QList>
#include <QVariant>

class vtkSMPropertyGroup;
class pqColorMapModel;

/// pqColorOpacityEditorWidget provides an in-line editor widget for editing the
/// color and opacity transfer functions. The property group is expected to have
/// properties with the following functions. If any of the optional properties
/// are missing, then the corresponding widgets are hidden.
/// \li "XRGBPoints"           :- property with (x,r,g,b) tuples that is
///                               controlled by a color-transfer function editor
///                               (pqTransferFunctionWidget).
/// \li "ScalarOpacityFunction":- (optional) proxy-property referring to a proxy with
///                               "Points" property with (x,a,m,s) tuples that
///                               is controlled by an opacity-transfer function
///                               editor (pqTransferFunctionWidget).
/// \li "EnableOpacityMapping" :- (optional) property used to enable
///                               opacity mapping for surfaces. Controlled by a
///                               checkbox in the Widget.
/// \li "UseLogScale"          :- (optional) property used to enable/disable log mapping
///                               for colors.
/// \li "LockScalarRange       :- (optional) property used to control if the application
///                               resets transfer function as and when needed.
/// UseLogScale.
/// Caveats:
/// \li Opacity editor:- pqColorOpacityEditorWidget shows an opacity editor widget.
/// Typically, opacity function is optional and used only when
/// "EnableOpacityMapping" is property is ON. However, in cases of Volume
/// rendering, the EnableOpacityMapping has no effect.
class PQAPPLICATIONCOMPONENTS_EXPORT pqColorOpacityEditorWidget :
  public pqPropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> xrgbPoints READ xrgbPoints WRITE setXrgbPoints)
  Q_PROPERTY(QList<QVariant> xvmsPoints READ xvmsPoints WRITE setXvmsPoints)
  Q_PROPERTY(bool useLogScale READ useLogScale WRITE setUseLogScale)
  Q_PROPERTY(bool lockScalarRange READ lockScalarRange WRITE setLockScalarRange)
  typedef pqPropertyWidget Superclass;
public:
  pqColorOpacityEditorWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent=0);
  virtual ~pqColorOpacityEditorWidget();

  /// Returns the current list of control points for the color transfer
  /// function. This a list of 4-tuples.
  QList<QVariant> xrgbPoints() const;

  /// Returns the current list of control points for the opacity
  /// function. This a list of 4-tuples.
  QList<QVariant> xvmsPoints() const;

  /// Returns the value for use-log-scale.
  bool useLogScale() const;

  /// Returns true if the scalar range is locked.
  bool lockScalarRange() const;

public slots:
  /// Sets the xvmsPoints that control the opacity transfer function.
  void setXvmsPoints(const QList<QVariant>&);

  /// Sets the xrgbPoints that control the color transfer function.
  void setXrgbPoints(const QList<QVariant>&);

  /// Set whether to use-log scale.
  void setUseLogScale(bool value);

  /// Set whether the scalar range must be locked.
  void setLockScalarRange(bool val);

  /// Reset the transfer function ranges to active data source.
  void resetRangeToData();

  /// Reset the transfer function ranges to custom values.
  void resetRangeToCustom();
  void resetRangeToCustom(double min, double max);

  /// Reset the transfer function ranges to temporal range for active data
  /// source.
  void resetRangeToDataOverTime();

  /// Inverts the transfer functions.
  void invertTransferFunctions();

  /// pick a preset.
  void choosePreset(const pqColorMapModel* add_new=NULL);

  /// save current transfer function as preset.
  void saveAsPreset();

signals:
  /// Signal fired when the xrgbPoints change.
  void xrgbPointsChanged();

  /// Signal fired when the xvmsPoints change.
  void xvmsPointsChanged();

  /// Signal fired when useLogScale changes.
  void useLogScaleChanged();

  /// Signal fired when lockScalarRange changes.
  void lockScalarRangeChanged();

protected slots:
  /// slots called when the current point changes on the two internal
  /// pqTransferFunctionWidget widgets.
  void opacityCurrentChanged(vtkIdType);
  void colorCurrentChanged(vtkIdType);

  /// updates the panel to show/hide advanced settings
  void updatePanel();

  /// updates the text shown in the "current data" input.
  void updateCurrentData();

  /// update the transfer function with the text in CurrentDataValue text field.
  void currentDataEdited();

  /// apply a present.
  void applyPreset(const pqColorMapModel*);

  /// Ensures that the color-swatches for indexedColors are shown only when this
  /// is set to true.
  void updateIndexedLookupState();

  /// called when the use-log-scale checkbox is clicked by the user. We then add
  /// extra logic to valid ranges convert the color map to log/linear space.
  void useLogScaleClicked(bool);

private:
  Q_DISABLE_COPY(pqColorOpacityEditorWidget);

  class pqInternals;
  pqInternals* Internals;
};

#endif
