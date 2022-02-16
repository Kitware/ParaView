/*=========================================================================

   Program: ParaView
   Module:  pqMaterialEditor.h

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
#ifndef pqMaterialEditor_h
#define pqMaterialEditor_h

#include "pqApplicationComponentsModule.h"

#include <QWidget>

#include <string> // for std::string

class vtkOSPRayMaterialLibrary;
class vtkSMProxy;

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
  pqMaterialEditor(QWidget* parent = nullptr);
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
  void removeProperty();
  void removeAllProperties();

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
