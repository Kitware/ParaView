// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  pqViewResolutionPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqViewResolutionPropertyWidget() override;

  ///@{
  /**
   * Overridden to clear the state for the highlight state for
   * "reset-to-domain" button. The implementation simply fires
   * `clearHighlight` signal.
   */
  void apply() override;
  void reset() override;
  ///@}

Q_SIGNALS:
  ///@{
  /**
   * internal signals used to highlight (or not) the "reset-to-domain" button.
   */
  void highlightResetButton();
  void clearHighlight();
  ///@}

private Q_SLOTS:
  void resetButtonClicked();
  void scale(double factor);
  void applyPreset();
  void applyRecent();
  void widthTextEdited(const QString&);
  void heightTextEdited(const QString&);
  void lockAspectRatioToggled(bool);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqViewResolutionPropertyWidget);
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
