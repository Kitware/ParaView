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
#include "pqMaterialEditor.h"
#include "ui_pqMaterialEditor.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkMath.h"
#include "vtkPVMaterial.h"
#include "vtkPVMaterialLibrary.h"
#include "vtkProcessModule.h"
#include "vtkSMMaterialLibraryProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayMaterialLibrary.h"
#endif

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqLoadMaterialsReaction.h"
#include "pqMaterialAttributesDelegate.h"
#include "pqNewMaterialDialog.h"
#include "pqObjectBuilder.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

#include <QMetaProperty>
#include <QSet>
#include <QStandardItemModel>
#include <QVector2D>
#include <QVector3D>
#include <QVector>

#include <sstream>

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
//-----------------------------------------------------------------------------
pqMaterialProxyModel::pqMaterialProxyModel(QObject* p)
  : QAbstractTableModel(p)
{
}

//-----------------------------------------------------------------------------
void pqMaterialProxyModel::reset()
{
  this->beginResetModel();
  this->endResetModel();
}

//-----------------------------------------------------------------------------
QVariant pqMaterialProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
      case 0:
        return "Name";
      case 1:
        return "Value";
    }
  }

  return QAbstractTableModel::headerData(section, orientation, role);
}

//-----------------------------------------------------------------------------
int pqMaterialProxyModel::rowCount(const QModelIndex& parent) const
{
  if (!this->Proxy)
  {
    return 0;
  }
  return vtkSMPropertyHelper(this->Proxy, "DoubleVariables").GetNumberOfElements() / 2;
}

//-----------------------------------------------------------------------------
std::string pqMaterialProxyModel::getMaterialAttributeName(const int row) const
{
  return vtkSMPropertyHelper(this->Proxy, "DoubleVariables").GetAsString(row * 2);
}

//-----------------------------------------------------------------------------
std::string pqMaterialProxyModel::getMaterialType() const
{
  return vtkSMPropertyHelper(this->Proxy, "Type").GetAsString();
}

//-----------------------------------------------------------------------------
QVariant pqMaterialProxyModel::data(const QModelIndex& index, int role) const
{
  if (index.column() == 0)
  {
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
      return QString(this->getMaterialAttributeName(index.row()).c_str());
    }
    if (this->IsSameRole(role, ExtendedItemDataRole::PropertyValue))
    {
      return QString(this->getMaterialType().c_str());
    }
  }
  if (index.column() == 1 && this->IsSameRole(role, ExtendedItemDataRole::PropertyValue))
  {
    std::string attName = this->getMaterialAttributeName(index.row());

    auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();
    std::string matType = this->getMaterialType();
    auto attType = dic.at(matType).at(attName);

    switch (attType)
    {
      case vtkOSPRayMaterialLibrary::ParameterType::BOOLEAN:
        return static_cast<bool>(
          vtkSMPropertyHelper(this->Proxy, "DoubleVariables").GetAsInt(index.row() * 2 + 1));
      case vtkOSPRayMaterialLibrary::ParameterType::FLOAT:
        return vtkSMPropertyHelper(this->Proxy, "DoubleVariables").GetAsDouble(index.row() * 2 + 1);
      case vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT:
        return vtkMath::ClampValue(
          vtkSMPropertyHelper(this->Proxy, "DoubleVariables").GetAsDouble(index.row() * 2 + 1), 0.0,
          1.0);
      case vtkOSPRayMaterialLibrary::ParameterType::TEXTURE:
        return QString(
          vtkSMPropertyHelper(this->Proxy, "DoubleVariables").GetAsString(index.row() * 2 + 1));
      case vtkOSPRayMaterialLibrary::ParameterType::VEC2:
      {
        std::array<double, 2> data;
        std::stringstream ss(
          vtkSMPropertyHelper(this->Proxy, "DoubleVariables").GetAsString(index.row() * 2 + 1));
        for (double& value : data)
        {
          ss >> value;
        }

        QVector2D vec;
        vec.setX(data[0]);
        vec.setY(data[1]);
        return vec;
      }
      case vtkOSPRayMaterialLibrary::ParameterType::VEC3:
      {
        std::array<double, 3> data;
        std::stringstream ss(
          vtkSMPropertyHelper(this->Proxy, "DoubleVariables").GetAsString(index.row() * 2 + 1));
        for (double& value : data)
        {
          ss >> value;
        }

        QVector3D vec;
        vec.setX(data[0]);
        vec.setY(data[1]);
        vec.setZ(data[2]);
        return vec;
      }
      case vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB:
      {
        std::array<double, 3> data;
        std::stringstream ss(
          vtkSMPropertyHelper(this->Proxy, "DoubleVariables").GetAsString(index.row() * 2 + 1));
        for (double& value : data)
        {
          ss >> value;
        }

        QColor col;
        col.setRedF(vtkMath::ClampValue(data[0], 0.0, 1.0));
        col.setGreenF(vtkMath::ClampValue(data[1], 0.0, 1.0));
        col.setBlueF(vtkMath::ClampValue(data[2], 0.0, 1.0));
        return col;
      }
      default:
        break;
    }
  }
  return QString();
}

//-----------------------------------------------------------------------------
bool pqMaterialProxyModel::setData(const QModelIndex& index, const QVariant& variant, int role)
{
  if (index.column() == 0 && role == Qt::EditRole)
  {
    vtkSMPropertyHelper(this->Proxy, "DoubleVariables")
      .Set(index.row() * 2, variant.toString().toLocal8Bit().data());
    this->Proxy->UpdateVTKObjects();

    QModelIndex sibling = index.sibling(index.row(), 1); // emit for value too
    emit this->dataChanged(index, sibling);
    return true;
  }
  if (index.column() == 1 && this->IsSameRole(role, ExtendedItemDataRole::PropertyValue))
  {
    std::stringstream ss;
    switch (static_cast<QMetaType::Type>(variant.type()))
    {
      case QMetaType::QVector2D:
      {
        QVector2D vec = variant.value<QVector2D>();
        ss << vec.x() << " " << vec.y();
      }
      break;
      case QMetaType::QVector3D:
      {
        QVector3D vec = variant.value<QVector3D>();
        ss << vec.x() << " " << vec.y() << " " << vec.z();
      }
      break;
      case QMetaType::QColor:
      {
        QColor col = variant.value<QColor>();
        ss << col.redF() << " " << col.greenF() << " " << col.blueF();
      }
      break;
      default:
        ss << variant.toString().toStdString();
    }
    vtkSMPropertyHelper(this->Proxy, "DoubleVariables").Set(index.row() * 2 + 1, ss.str().c_str());

    this->Proxy->UpdateVTKObjects();

    emit this->dataChanged(index, index);
    return true;
  }
  return QAbstractTableModel::setData(index, variant, role);
}
#endif

class vtkSMProxyManagerRegisterObserver : public vtkCommand
{
public:
  static vtkSMProxyManagerRegisterObserver* New()
  {
    return new vtkSMProxyManagerRegisterObserver();
  }

  void Execute(vtkObject* vtkNotUsed(obj), unsigned long vtkNotUsed(event), void* data) override
  {
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
    vtkSMProxyManager::RegisteredProxyInformation* info =
      static_cast<vtkSMProxyManager::RegisteredProxyInformation*>(data);
    if (strcmp(info->GroupName, "materials") == 0)
    {
      vtkPVMaterial::SafeDownCast(info->Proxy->GetClientSideObject())
        ->SetLibrary(this->Editor->materialLibrary());
      this->Editor->updateMaterialList();
    }
#endif
  }

  pqMaterialEditor* Editor;
};

//-----------------------------------------------------------------------------
class pqMaterialEditor::pqInternals
{
public:
  Ui::MaterialEditor Ui;

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  pqMaterialProxyModel AttributesModel;
#endif

  pqInternals(pqMaterialEditor* self)
  {
    this->Ui.setupUi(self);

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
    this->Ui.PropertiesView->setModel(&this->AttributesModel);
#endif
    this->Ui.PropertiesView->horizontalHeader()->setHighlightSections(false);
    this->Ui.PropertiesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->Ui.PropertiesView->horizontalHeader()->setStretchLastSection(true);
    this->Ui.PropertiesView->verticalHeader()->hide();
  }

  ~pqInternals() = default;
};

//-----------------------------------------------------------------------------
pqMaterialEditor::pqMaterialEditor(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqMaterialEditor::pqInternals(this))
{
  this->Internals->Ui.PropertiesView->setItemDelegate(new pqMaterialAttributesDelegate(this));

  // materials
  QObject::connect(this->Internals->Ui.AddMaterial, SIGNAL(clicked()), this, SLOT(addMaterial()));
  QObject::connect(
    this->Internals->Ui.RemoveMaterial, SIGNAL(clicked()), this, SLOT(removeMaterial()));
  QObject::connect(
    this->Internals->Ui.LoadMaterials, SIGNAL(clicked()), this, SLOT(loadMaterials()));

  // attributes
  QObject::connect(this->Internals->Ui.SelectMaterial, SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(updateCurrentMaterial(const QString&)));
  QObject::connect(this->Internals->Ui.AddProperty, SIGNAL(clicked()), this, SLOT(addProperty()));
  QObject::connect(
    this->Internals->Ui.RemoveProperty, SIGNAL(clicked()), this, SLOT(removeProperty()));
  QObject::connect(
    this->Internals->Ui.DeleteProperties, SIGNAL(clicked()), this, SLOT(removeAllProperties()));

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  QObject::connect(&this->Internals->AttributesModel,
    SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(propertyChanged(const QModelIndex&, const QModelIndex&)));
#endif

  QObject::connect(
    &pqActiveObjects::instance(), &pqActiveObjects::serverChanged, [=](pqServer* server) {
      if (server)
      {
        vtkNew<vtkSMProxyManagerRegisterObserver> observer;
        observer->Editor = this;
        server->proxyManager()->AddObserver(vtkCommand::RegisterEvent, observer);
        // This will clear the material editor when a new connection occurs
        this->Internals->Ui.SelectMaterial->clear();
      }
    });

  this->updateMaterialList();
}

//-----------------------------------------------------------------------------
pqMaterialEditor::~pqMaterialEditor()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
QString pqMaterialEditor::currentMaterialName()
{
  return this->Internals->Ui.SelectMaterial->currentText();
}

//-----------------------------------------------------------------------------
vtkOSPRayMaterialLibrary* pqMaterialEditor::materialLibrary()
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkSMMaterialLibraryProxy* mlp =
    vtkSMMaterialLibraryProxy::SafeDownCast(this->materialLibraryProxy());
  if (mlp)
  {
    return vtkOSPRayMaterialLibrary::SafeDownCast(
      vtkPVMaterialLibrary::SafeDownCast(mlp->GetClientSideObject())->GetMaterialLibrary());
  }
#endif
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqMaterialEditor::materialLibraryProxy()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    return nullptr;
  }

  return server->proxyManager()->FindProxy("materiallibrary", "materials", "MaterialLibrary");
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqMaterialEditor::materialProxy(const QString& matName)
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  vtkSMSessionProxyManager* pxm = server->proxyManager();

  vtkNew<vtkCollection> collection;
  pxm->GetProxies("materials", collection);

  collection->InitTraversal();
  vtkObject* obj = nullptr;
  while ((obj = collection->GetNextItemAsObject()) != nullptr)
  {
    vtkSMProxy* mp = vtkSMProxy::SafeDownCast(obj);

    if (matName.toStdString() == vtkSMPropertyHelper(mp, "Name").GetAsString())
    {
      return mp;
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::addMaterial()
{
  pqNewMaterialDialog* dialog = new pqNewMaterialDialog(pqCoreUtilities::mainWidget());
  dialog->setWindowTitle("New Material");
  dialog->setMaterialLibrary(this->materialLibrary());
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();

  QObject::connect(dialog, &pqNewMaterialDialog::accepted, [=]() {
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    pqServer* server = pqActiveObjects::instance().activeServer();
    vtkSMProxy* mp = builder->createProxy("materials", "Material", server, "materials");

    const std::string matName = this->generateValidMaterialName(dialog->name().toStdString());

    vtkSMPropertyHelper(mp, "Name").Set(matName.c_str());
    vtkSMPropertyHelper(mp, "Type").Set(dialog->type().toLocal8Bit().data());
    mp->UpdateVTKObjects();

    vtkSMProxy* mlp = this->materialLibraryProxy();
    vtkSMPropertyHelper(mlp, "Materials").Add(mp);
    mlp->UpdateVTKObjects();

    this->updateMaterialList();
    this->Internals->Ui.SelectMaterial->setCurrentText(QString(matName.c_str()));
  });
}

//-----------------------------------------------------------------------------
std::string pqMaterialEditor::generateValidMaterialName(const std::string& name)
{
  vtkOSPRayMaterialLibrary* ml = this->materialLibrary();
  const auto& names = ml->GetMaterialNames();
  std::string res = name;

  auto findName = names.find(name);
  if (findName != names.end())
  {
    int counter = 0;
    do
    {
      res = std::string(name).append("_").append(std::to_string(++counter));
      findName = names.find(res);
    } while (findName != names.end());
  }

  return res;
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::removeMaterial()
{
  vtkOSPRayMaterialLibrary* ml = this->materialLibrary();

  if (ml)
  {
    vtkSMProxy* mp = this->materialProxy(this->currentMaterialName());

    if (mp)
    {
      vtkSMProxy* mlp = this->materialLibraryProxy();
      vtkSMPropertyHelper(mlp, "Materials").Remove(mp);
      mlp->UpdateVTKObjects();

      // remove material proxy
      pqServer* server = pqActiveObjects::instance().activeServer();
      server->proxyManager()->UnRegisterProxy(mp->GetXMLGroup(), mp->GetXMLName(), mp);

      this->updateMaterialList();
    }
  }
}

//-----------------------------------------------------------------------------
std::vector<std::string> pqMaterialEditor::availableParameters()
{
  std::vector<std::string> availableList;

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayMaterialLibrary* ml = this->materialLibrary();

  const QString& currentText = this->currentMaterialName();
  if (ml && !currentText.isEmpty())
  {
    auto matName = currentText.toStdString();
    auto matType = ml->LookupImplName(matName);
    auto usedVariable = ml->GetDoubleShaderVariableList(matName);
    auto usedTexture = ml->GetTextureList(matName);

    auto& allParams = vtkOSPRayMaterialLibrary::GetParametersDictionary().at(matType);
    // Filter all available parameters so only the relevant ones are left
    for (auto& p : allParams)
    {
      if (p.second != vtkOSPRayMaterialLibrary::ParameterType::FLOAT_DATA &&
        p.first.find(".transform") == std::string::npos &&
        std::find(usedVariable.begin(), usedVariable.end(), p.first) == usedVariable.end() &&
        std::find(usedTexture.begin(), usedTexture.end(), p.first) == usedTexture.end())
      {
        availableList.push_back(p.first);
      }
    }
  }
#endif
  return availableList;
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::loadMaterials()
{
  pqLoadMaterialsReaction::loadMaterials();
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::addNewProperty(vtkSMProxy* proxy, const QString& prop)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();

  unsigned int nbElem = vtkSMPropertyHelper(proxy, "DoubleVariables").GetNumberOfElements();
  vtkSMPropertyHelper(proxy, "DoubleVariables").SetNumberOfElements(nbElem + 2);
  vtkSMPropertyHelper(proxy, "DoubleVariables").Set(nbElem, prop.toLocal8Bit().data());

  std::string matType = vtkSMPropertyHelper(proxy, "Type").GetAsString();

  switch (dic.at(matType).at(prop.toStdString()))
  {
    case vtkOSPRayMaterialLibrary::ParameterType::BOOLEAN:
      vtkSMPropertyHelper(proxy, "DoubleVariables").Set(nbElem + 1, "1");
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::FLOAT:
      vtkSMPropertyHelper(proxy, "DoubleVariables").Set(nbElem + 1, "1.5");
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT:
      vtkSMPropertyHelper(proxy, "DoubleVariables").Set(nbElem + 1, "1");
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::VEC2:
      vtkSMPropertyHelper(proxy, "DoubleVariables").Set(nbElem + 1, "0 0");
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::VEC3:
      vtkSMPropertyHelper(proxy, "DoubleVariables").Set(nbElem + 1, "0 0 0");
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB:
      vtkSMPropertyHelper(proxy, "DoubleVariables").Set(nbElem + 1, "1 1 1");
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::FLOAT_DATA:
      vtkSMPropertyHelper(proxy, "DoubleVariables").Set(nbElem + 1, "");
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::TEXTURE:
      vtkSMPropertyHelper(proxy, "DoubleVariables").Set(nbElem + 1, "");
      break;
    default:
      vtkGenericWarningMacro("Material property " << prop.toStdString() << " has unsupported type");
      break;
  }

  this->Internals->AttributesModel.reset();
#endif
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::addProperty()
{
  auto params = this->availableParameters();
  if (!params.empty())
  {
    QString matName = this->currentMaterialName();
    QString matType = this->Internals->Ui.SelectMaterial->currentData().toString();

    vtkSMProxy* mp = this->materialProxy(matName);

    this->addNewProperty(mp, QString::fromStdString(params[0]));

    mp->UpdateVTKObjects();
    this->updateCurrentMaterial(matName);
  }
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::removeProperties(vtkSMProxy* proxy, const QSet<QString>& variables)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  unsigned int nbElem = vtkSMPropertyHelper(proxy, "DoubleVariables").GetNumberOfElements();
  unsigned int newNbElem = 0;
  for (unsigned int i = 0; i < nbElem; i += 2)
  {
    const char* varName = vtkSMPropertyHelper(proxy, "DoubleVariables").GetAsString(i);
    if (!variables.contains(varName))
    {
      const char* value = vtkSMPropertyHelper(proxy, "DoubleVariables").GetAsString(i + 1);
      vtkSMPropertyHelper(proxy, "DoubleVariables").Set(newNbElem, varName);
      vtkSMPropertyHelper(proxy, "DoubleVariables").Set(newNbElem + 1, value);

      newNbElem += 2;
    }
  }
  vtkSMPropertyHelper(proxy, "DoubleVariables").SetNumberOfElements(newNbElem);

  proxy->UpdateVTKObjects();
#endif
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::removeProperty()
{
  // Tache 8.1.? : RemoveProperty reset all properties
  vtkOSPRayMaterialLibrary* ml = this->materialLibrary();

  if (ml)
  {
    QItemSelectionModel* selectionModel = this->Internals->Ui.PropertiesView->selectionModel();
    if (selectionModel->hasSelection())
    {
      QModelIndexList selectedCells = selectionModel->selectedIndexes();

      QSet<QString> selectedVariables;
      for (auto index : selectedCells)
      {
        selectedVariables.insert(index.sibling(index.row(), 0).data(Qt::EditRole).toString());
      }

      QString matName = this->currentMaterialName();
      vtkSMProxy* mp = this->materialProxy(matName);

      this->removeProperties(mp, selectedVariables);
      this->updateCurrentMaterial(matName);
    }
  }
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::removeAllProperties()
{
  vtkOSPRayMaterialLibrary* ml = this->materialLibrary();

  if (ml)
  {
    QString matName = this->currentMaterialName();
    vtkSMProxy* mp = this->materialProxy(matName);
    if (!mp)
    {
      return;
    }
    vtkSMPropertyHelper(mp, "DoubleVariables").RemoveAllValues();

    mp->UpdateVTKObjects();

    this->updateCurrentMaterial(matName);
  }
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::propertyChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
  // update material and render
  QString matName = this->currentMaterialName();

  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    return;
  }

  vtkSMSessionProxyManager* pxm = server->proxyManager();

  vtkNew<vtkCollection> collection;
  pxm->GetProxies("representations", collection);

  collection->InitTraversal();
  vtkObject* obj = nullptr;
  bool needRender = false;
  while ((obj = collection->GetNextItemAsObject()) != nullptr)
  {
    vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(obj);

    if (matName == vtkSMPropertyHelper(proxy, "OSPRayMaterial").GetAsString())
    {
      vtkSMPropertyHelper(proxy, "OSPRayMaterial").Set("None");
      proxy->UpdateVTKObjects();
      vtkSMPropertyHelper(proxy, "OSPRayMaterial").Set(matName.toLocal8Bit().data());
      proxy->UpdateVTKObjects();

      needRender = true;
    }
  }

  if (needRender)
  {
    pqApplicationCore* app = pqApplicationCore::instance();
    pqServerManagerModel* smModel = app->getServerManagerModel();
    QList<pqView*> views = smModel->findItems<pqView*>();

    for (auto v : views)
    {
      v->render();
    }
  }
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::updateMaterialList()
{
  vtkSMProxy* mlp = this->materialLibraryProxy();

  if (mlp)
  {
    int nbMaterials = vtkSMPropertyHelper(mlp, "Materials").GetNumberOfElements();

    this->Internals->Ui.SelectMaterial->clear();

    for (int i = 0; i < nbMaterials; i++)
    {
      vtkSMProxy* matProxy = vtkSMPropertyHelper(mlp, "Materials").GetAsProxy(i);
      this->Internals->Ui.SelectMaterial->addItem(
        vtkSMPropertyHelper(matProxy, "Name").GetAsString());
    }
  }
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::updateCurrentMaterial(const QString& label)
{
  vtkSMProxy* proxy = nullptr;

  // material type name label update
  std::string materialTypeLabel = "Material Type: ";

  if (!label.isEmpty())
  {
    proxy = this->materialProxy(label);
    materialTypeLabel += vtkSMPropertyHelper(proxy, "Type").GetAsString();
  }
  else
  {
    materialTypeLabel += "<none>";
  }

  this->Internals->Ui.TypeLabel->setText(materialTypeLabel.c_str());

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  this->Internals->AttributesModel.setProxy(proxy);
  this->Internals->AttributesModel.reset();
  this->propertyChanged(QModelIndex(), QModelIndex());
#endif
}
