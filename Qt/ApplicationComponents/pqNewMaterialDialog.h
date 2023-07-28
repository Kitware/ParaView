// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqNewMaterialDialog_h
#define pqNewMaterialDialog_h

#include "pqApplicationComponentsModule.h"
#include "pqDialog.h"

class vtkOSPRayMaterialLibrary;

/**
 * @brief pqNewMaterialDialog is a dialog window that is used to create a new
 * material in the material editor widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqNewMaterialDialog : public pqDialog
{
  Q_OBJECT
  typedef pqDialog Superclass;

public:
  pqNewMaterialDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  ~pqNewMaterialDialog() override;

  /**
   * Set the OSPRay material library used to check if the material name is
   * available in OSPRay.
   */
  void setMaterialLibrary(vtkOSPRayMaterialLibrary* lib);
  /**
   * Return the name of the material
   */
  const QString& name() { return this->Name; }
  /**
   * Return the type of the material
   */
  const QString& type() { return this->Type; }

public Q_SLOTS:
  /**
   * Store the name and type of the material after accept.
   * This slot is connected in pqMaterialEditor to add a new material to
   * the library.
   */
  void accept() override;

protected:
  vtkOSPRayMaterialLibrary* MaterialLibrary;

  QString Name;
  QString Type;

private:
  Q_DISABLE_COPY(pqNewMaterialDialog)
  class pqInternals;
  pqInternals* Internals;
  friend class pqInternals;
};

#endif
