// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTextureComboBox_h
#define pqTextureComboBox_h

#include "pqComponentsModule.h"
#include "vtkNew.h"

#include <QComboBox>

/**
 * This is a ComboBox that is used on the display tab to select available
 * textures. It can be used with Representations, Sources and Views.
 * It provides the user the optional feature of loading new images as
 * textures directly from the combobox. One can choose to disable this
 * feature by setting canLoadNew to false in the constructor. If omitted
 * them CanLoadNew is true.
 */
class vtkSMProxyGroupDomain;
class vtkSMProxy;
class vtkEventQtSlotConnect;
class PQCOMPONENTS_EXPORT pqTextureComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;

public:
  pqTextureComboBox(vtkSMProxyGroupDomain* domain, QWidget* parent = nullptr);
  pqTextureComboBox(vtkSMProxyGroupDomain* domain, bool canLoadNew, QWidget* parent = nullptr);
  ~pqTextureComboBox() override = default;

  /**
   * Update the selected index in the combobox using the provided texture if it
   * is present in the combobox
   */
  void updateFromTexture(vtkSMProxy* texture);

Q_SIGNALS:

  /**
   * Emitted whenever the texture has been changed
   */
  void textureChanged(vtkSMProxy* texture);

protected:
  void loadTexture();
  bool loadTexture(const QString& filename);

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void onCurrentIndexChanged(int index);
  void updateTextures();
  void proxyRegistered(const QString& group, const QString&, vtkSMProxy* proxy);
  void proxyUnRegistered(const QString& group, const QString&, vtkSMProxy* proxy);

private:
  Q_DISABLE_COPY(pqTextureComboBox)

  vtkSMProxyGroupDomain* Domain;
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  QString GroupName;
  bool CanLoadNew;
};

#endif
