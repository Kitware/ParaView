// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnimatableProxyComboBox_h
#define pqAnimatableProxyComboBox_h

#include "pqComponentsModule.h"
#include "pqSMProxy.h"
#include <QComboBox>

class vtkSMProxy;
class pqPipelineSource;
class pqServerManagerModelItem;

/**
 * pqAnimatableProxyComboBox is a combo box that can list the animatable
 * proxies.  All pqPipelineSources are automatically in this list
 * Any other proxies must be manually added.
 */
class PQCOMPONENTS_EXPORT pqAnimatableProxyComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;

public:
  pqAnimatableProxyComboBox(QWidget* parent = nullptr);
  ~pqAnimatableProxyComboBox() override;

  /**
   * Returns the current source
   */
  vtkSMProxy* getCurrentProxy() const;

  void addProxy(int index, const QString& label, vtkSMProxy*);
  void removeProxy(const QString& label);
  int findProxy(vtkSMProxy*);

protected Q_SLOTS:
  void onSourceAdded(pqPipelineSource* src);
  void onSourceRemoved(pqPipelineSource* src);
  void onNameChanged(pqServerManagerModelItem* src);
  void onCurrentSourceChanged(int idx);

Q_SIGNALS:
  void currentProxyChanged(vtkSMProxy*);

private:
  Q_DISABLE_COPY(pqAnimatableProxyComboBox)
};

#endif
