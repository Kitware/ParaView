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
#ifndef pqMoleculePropertyWidget_h
#define pqMoleculePropertyWidget_h

#include "pqPropertyGroupWidget.h"

struct MapperParameters;
class vtkSMPropertyGroup;
class QWidget;

/**
 * @class pqMoleculePropertyWidget
 * @brief Expose molecule mapper parameters to the user.
 *
 * Some presets are defined for an easiest configuration.
 */
class pqMoleculePropertyWidget : public pqPropertyGroupWidget
{
  Q_OBJECT
  typedef pqPropertyGroupWidget Superclass;

public:
  pqMoleculePropertyWidget(
    vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject = 0);
  ~pqMoleculePropertyWidget() override = default;

  /**
   * Override to show/hide relevant widgets depending on the advanced properties status.
   */
  void updateWidget(bool showing_advanced_properties) override;

protected Q_SLOTS:
  //@{
  /**
   * Show/hide widgets depending on the states of other widgets.
   * Handle advanced proprerties visibility.
   */
  void updateBondWidgetsVisibility();
  void updateAtomWidgetsVisibility();
  void updateAtomicRadiusWidgetsVisibility();
  void updateBondColorWidgetVisibility();
  //@}

  /**
   * Apply selected preset to the proxy
   */
  void onPresetChanged(int);

  /**
   * Reset the preset widget.
   * If a property has changed, preset widget should display '(no preset)'.
   */
  void resetPreset();

  //@{
  /**
   * Update the atom/bond radius sliders bounds.
   */
  void onScaleAtomFactorChanged(double scale);
  void onResetAtomFactorToggled();
  void onScaleBondRadiusChanged(double scale);
  void onResetBondRadiusToggled();
  //@}

protected:
  /**
   * Set property documentation as widget tooltip.
   */
  void setDocumentationAsTooltip(vtkSMProperty* prop, QWidget* widget);

private:
  Q_DISABLE_COPY(pqMoleculePropertyWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
