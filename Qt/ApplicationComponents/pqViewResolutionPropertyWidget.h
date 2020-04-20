/*=========================================================================

   Program: ParaView
   Module:  pqViewResolutionPropertyWidget.h

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
#ifndef pqViewResolutionPropertyWidget_h
#define pqViewResolutionPropertyWidget_h

#include "pqApplicationComponentsModule.h" // needed for exports
#include "pqPropertyWidget.h"
#include <QScopedPointer> // needed for ivar

/**
 * @class pqViewResolutionPropertyWidget
 * @brief widget for view resolution.
 *
 * pqViewResolutionPropertyWidget is desined to be used with properties
 * that allow users to set resolution for things like views, images, movies
 * etc. It provides UI elements to lock aspect ratio, scale size up or down,
 * and reset to default.
 *
 * This widget works with any vtkSMIntVectorProperty with 2 elements interpreted as
 * (width, height).
 *
 * As specified in pqStandardPropertyWidgetInterface, to use this widget for
 * a property you can use `panel_widget="view_resolution"` attribute
 * in ServerManager XML.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqViewResolutionPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqViewResolutionPropertyWidget(vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = 0);
  ~pqViewResolutionPropertyWidget() override;

  //@{
  /**
   * Overridden to clear the state for the highlight state for
   * "reset-to-domain" button. The implementation simply fires
   * `clearHighlight` signal.
   */
  void apply() override;
  void reset() override;
  //@}

Q_SIGNALS:
  //@{
  /**
   * internal signals used to highlight (or not) the "reset-to-domain" button.
   */
  void highlightResetButton();
  void clearHighlight();
  //@}

private Q_SLOTS:
  void resetButtonClicked();
  void scale(double factor);
  void widthTextEdited(const QString&);
  void heightTextEdited(const QString&);
  void lockAspectRatioToggled(bool);

private:
  Q_DISABLE_COPY(pqViewResolutionPropertyWidget);
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
