// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqLinksEditor_h
#define pqLinksEditor_h

#include <QDialog>
#include <QListWidgetItem>
#include <QModelIndex>
#include <QScopedPointer>

#include "pqComponentsModule.h"
#include "pqLinksModel.h"

namespace Ui
{
class pqLinksEditor;
}

/**
 * A Qt dialog for editing a property/proxy/camera link.
 * Two proxies can be selected, and if property type is
 * selected, then two properties can be selected as well.
 */
class PQCOMPONENTS_EXPORT pqLinksEditor : public QDialog
{
  Q_OBJECT
  typedef QDialog base;

public:
  /**
   * Create a link editor to create/edit a link.
   * Initial values are retrieved from the provided vtkSMLink.
   */
  pqLinksEditor(vtkSMLink* link, QWidget* p = nullptr);

  /**
   * Destroy this dialog
   */
  ~pqLinksEditor() override;

  /**
   * Get the name of the link
   */
  QString linkName();

  /**
   * get the type of link
   */
  pqLinksModel::ItemType linkType();

  /**
   * Get the first selected proxy
   */
  vtkSMProxy* selectedProxy1();

  /**
   * Get the second selected proxy;
   */
  vtkSMProxy* selectedProxy2();

  /**
   * Get the first selected property
   */
  QString selectedProperty1();

  /**
   * Get the second selected property
   */
  QString selectedProperty2();

  /**
   * Get the check state of interactive view link check box
   */
  bool interactiveViewLinkChecked();

  /**
   * Get the check state of camera widget view link check box
   */
  bool cameraWidgetViewLinkChecked();

  /**
   * Get the check state of convert to indices check box
   */
  bool convertToIndicesChecked();

private Q_SLOTS:
  void currentProxy1Changed(const QModelIndex& cur, const QModelIndex&);
  void currentProxy2Changed(const QModelIndex& cur, const QModelIndex&);

  void currentProperty1Changed(QListWidgetItem* item);
  void currentProperty2Changed(QListWidgetItem* item);

  void updateSelectedProxies();
  void updateEnabledState();

private: // NOLINT(readability-redundant-access-specifiers)
  class pqLinksEditorProxyModel;
  void updatePropertyList(QListWidget* tw, vtkSMProxy* proxy);

  QScopedPointer<Ui::pqLinksEditor> Ui;

  pqLinksEditorProxyModel* Proxy1Model = nullptr;
  pqLinksEditorProxyModel* Proxy2Model = nullptr;

  vtkSMProxy* SelectedProxy1 = nullptr;
  vtkSMProxy* SelectedProxy2 = nullptr;
  QString SelectedProperty1;
  QString SelectedProperty2;
};

#endif
