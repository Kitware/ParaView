/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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

#include <QAbstractTableModel>
#include <QWidget>

class vtkOSPRayMaterialLibrary;
class vtkSMProxy;
class pqDataRepresentation;

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
/**
 * The Qt model associated with the 2D array representation of the material properties
 */
class pqMaterialProxyModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  /**
   * Strong type extension of the existing Qt::ItemDataRole This is used to store the QVariant value
   * for the properties
   */
  enum class ExtendedItemDataRole
  {
    PropertyValue = Qt::UserRole + 1
  };

  /**
   * Default constructor initialize the Qt hierarchy
   */
  pqMaterialProxyModel(QObject* p = nullptr);

  /**
   * Defaulted destructor for inheritance
   */
  ~pqMaterialProxyModel() override = default;

  /**
   * Sets the material proxy whose property will be displayed
   */
  void setProxy(vtkSMProxy* proxy) { this->Proxy = proxy; }

  /**
   * Returns the flags associated with this model
   */
  Qt::ItemFlags flags(const QModelIndex& idx) const override
  {
    return QAbstractTableModel::flags(idx) | Qt::ItemIsEditable;
  }

  /**
   * Triggers a global update of all the elements within the 2D array
   */
  void reset();

  /**
   * Construct the header data for this model
   */
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

  /**
   * Returns the row count of the 2D array. This corresponds to the number of properties holded by
   * the material
   */
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;

  /**
   * Returns the nomber of columns (two in our case)
   */
  int columnCount(const QModelIndex& parent = QModelIndex()) const override { return 2; }

  /**
   * Returns the material attribute name at row
   */
  std::string getMaterialAttributeName(const int row) const;

  /**
   * Query the proxy to get the material type
   */
  std::string getMaterialType() const;

  /**
   * Return the data at index with role
   */
  QVariant data(const QModelIndex& index, int role) const override;

  /**
   * Overrides the data at index and role with the input variant
   */
  bool setData(const QModelIndex& index, const QVariant& variant, int role) override;

private:
  bool IsSameRole(int role1, ExtendedItemDataRole role2) const
  {
    return role1 == static_cast<int>(role2);
  }

  vtkSMProxy* Proxy = nullptr;
};
#endif

/**
 * pqMaterialEditor is a widget that can be used to edit the materials.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqMaterialEditor : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqMaterialEditor(QWidget* parent = nullptr);
  ~pqMaterialEditor() override;

  void updateMaterialList();

  QString currentMaterialName();
  vtkSMProxy* materialLibraryProxy();
  vtkSMProxy* materialProxy(const QString& matName);
  vtkOSPRayMaterialLibrary* materialLibrary();
  std::vector<std::string> availableParameters();

protected slots:
  void updateCurrentMaterial(const QString&);

  void loadMaterials();

  void addMaterial();
  void removeMaterial();
  void attachMaterial();

  void synchronizeClientToServ();

  void addProperty();
  void removeProperty();
  void removeAllProperties();

  void propertyChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

protected:
  void removeProperties(vtkSMProxy* proxy, const QSet<QString>& variables);
  void addNewProperty(vtkSMProxy* proxy, const QString& prop);

  /**
   * Return a unique material name given a desired name.
   *
   * Checks for all the already existing materials and if one has the same
   * name, this function appends the suffix `_X` where `X` is the next unique
   * integer identifier.
   */
  std::string generateValidMaterialName(const std::string& name);

private:
  Q_DISABLE_COPY(pqMaterialEditor)
  class pqInternals;
  pqInternals* Internals;
  friend class pqInternals;
};

#endif
