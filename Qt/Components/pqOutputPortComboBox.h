// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqOutputPortComboBox_h
#define pqOutputPortComboBox_h

#include "pqComponentsModule.h"
#include <QComboBox>

class pqOutputPort;
class pqPipelineSource;
class pqServerManagerModelItem;

/**
 * pqOutputPortComboBox is a combo-box that shows all outputports for all
 * sources/filters.
 */
class PQCOMPONENTS_EXPORT pqOutputPortComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;

public:
  pqOutputPortComboBox(QWidget* parent = nullptr);
  ~pqOutputPortComboBox() override;

  /**
   * Enable/Disable changing of the combo-box selected index based on the
   * active source/port. Default is true i.e. enabled.
   */
  void setAutoUpdateIndex(bool val) { this->AutoUpdateIndex = val; }
  bool autoUpdateIndex() const { return this->AutoUpdateIndex; }

  /**
   * Makes is possible to add custom items to the combo-box.
   * \c port can be nullptr.
   */
  void addCustomEntry(const QString& label, pqOutputPort* port);

  /**
   * Returns the currently selected output port.
   */
  pqOutputPort* currentPort() const;

  /**
   * May be called once after creation to initialize the widget with already
   * existing sources.
   */
  void fillExistingPorts();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Sets the current index to be the one representing the indicated port.
   */
  void setCurrentPort(pqOutputPort* port);

Q_SIGNALS:
  /**
   * Fired when the current index changes.
   */
  void currentIndexChanged(pqOutputPort*);

private Q_SLOTS:
  /**
   * Called when a source's name might have changed.
   */
  void nameChanged(pqServerManagerModelItem* item);

  /**
   * Called when current in the server manager selection changes.
   */
  void portChanged(pqOutputPort* item);

  /**
   * Called when currentIndexChanged(int) is fired.
   * We fire currentIndexChanged(pqPipelineSource*) and
   */
  // currentIndexChanged(vtkSMProxy*);
  void onCurrentIndexChanged(int index);

  /**
   * Called when a new source is added.
   */
  void addSource(pqPipelineSource* source);

  /**
   * Called when a new source is removed.
   */
  void removeSource(pqPipelineSource* source);

protected:
  bool AutoUpdateIndex;

private:
  Q_DISABLE_COPY(pqOutputPortComboBox)
};

#endif
