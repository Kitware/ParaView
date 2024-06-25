// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqMultiBlockPropertiesStateWidget_h
#define pqMultiBlockPropertiesStateWidget_h

#include "pqComponentsModule.h"

#include "vtkSMColorMapEditorHelper.h"

#include <QList>
#include <QScopedPointer>
#include <QString>
#include <QWidget>

#include <string>
#include <vector>

class pqHighlightableToolButton;
class vtkSMProxy;

/**
 * @class pqMultiBlockPropertiesStateWidget
 * @brief A QWidget that tracks the state multi block property/ies, and allows to reset them.
 *
 * This widget is used to track the state of multiple properties of a multi block proxy.
 * It allows to reset the state of the properties. If a selector is provided, the widget
 * will only track the properties of the selected block, else it will track the properties
 * of the current selectors provided by SelectedBlockSelectors property of the proxy.
 */
class PQCOMPONENTS_EXPORT pqMultiBlockPropertiesStateWidget : public QWidget
{
  Q_OBJECT
public:
  typedef QWidget Superclass;

  pqMultiBlockPropertiesStateWidget(vtkSMProxy* repr, const QList<QString>& properties,
    int iconSize, QString selector = QString(), QWidget* parent = nullptr,
    Qt::WindowFlags f = Qt::WindowFlags());
  ~pqMultiBlockPropertiesStateWidget() override;

  using BlockPropertyState = vtkSMColorMapEditorHelper::BlockPropertyState;

  /**
   * Get the state of the properties.
   */
  BlockPropertyState getState() const;

  /*
   * Get the Pixmap of the current state.
   */
  QPixmap getStatePixmap(BlockPropertyState state) const;

  /**
   * Get the tooltip of the current state.
   */
  QString getStateToolTip(BlockPropertyState state) const;

  /**
   * Get the reset button.
   */
  pqHighlightableToolButton* getResetButton() const;

  /**
   * Get the properties being tracked.
   */
  std::vector<std::string> getProperties() const;

  /**
   * Get the selectors being tracked.
   */
  std::vector<std::string> getSelectors() const;

Q_SIGNALS:
  void stateChanged(BlockPropertyState newState);
  void selectedBlockSelectorsChanged();
  void startStateReset();
  void endStateReset();

private Q_SLOTS:
  void onSelectedBlockSelectorsChanged();
  void onPropertiesChanged();
  void onResetButtonClicked();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqMultiBlockPropertiesStateWidget)

  void updateState();

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif // pqMultiBlockPropertiesStateWidget_h
