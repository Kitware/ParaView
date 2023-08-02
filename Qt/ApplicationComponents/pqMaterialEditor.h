// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMaterialEditor_h
#define pqMaterialEditor_h

#include "pqApplicationComponentsModule.h"

#include <QWidget>

#include <string> // for std::string

class vtkOSPRayMaterialLibrary;
class vtkSMProxy;
class QDockWidget;

/**
 * @brief pqMaterialEditor is a widget that can be used to edit the OSPRay materials.
 *
 * This widget allows you to create or remove an OSPRay material.
 * Then, you can customize this material by adding or removing properties, such as the color,
 * the roughness, the textures... depending on the available properties that you can see
 * in vtkOSPRayMaterialLibrary.
 * Texture format supported are *.png, *.jpg, *.bmp and *.ppm.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqMaterialEditor : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqMaterialEditor(QWidget* parent = nullptr, QDockWidget* dockWidget = nullptr);
  ~pqMaterialEditor() override;

  /**
   * Clear the editor and fill the editor with the current created materials.
   */
  void updateMaterialList();

  /**
   * Return the name of the current selected material.
   */
  QString currentMaterialName();

  /**
   * Return the list of the available OSPRay parameters for the current material.
   */
  std::vector<std::string> availableParameters();

  /**
   * Strong type extension of the existing Qt::ItemDataRole.
   * This is used to store the QVariant value for the properties
   */
  enum class ExtendedItemDataRole
  {
    PropertyValue = Qt::UserRole + 1
  };

protected Q_SLOTS:
  void updateCurrentMaterial(const std::string&);
  void updateCurrentMaterialWithIndex(int index);

  void loadMaterials();

  void addMaterial();
  void removeMaterial();
  void attachMaterial();
  void saveMaterials();

  void addProperty();

  void propertyChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

protected:
  /**
   * Return a unique material name given a desired name.
   *
   * Checks for all the already existing materials and if one has the same
   * name, this function appends the suffix `_X` where `X` is the next unique
   * integer identifier.
   */
  std::string generateValidMaterialName(const std::string& name);

  /**
   * Overriden to warn user when material editor is not usable.
   */
  void showEvent(QShowEvent* event) override;

private:
  Q_DISABLE_COPY(pqMaterialEditor)
  class pqInternals;
  pqInternals* Internals;
  friend class pqInternals;

  bool HasWarnedUser = true;
};

#endif
