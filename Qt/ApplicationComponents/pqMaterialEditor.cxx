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

#include "vtkBMPReader.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkJPEGReader.h"
#include "vtkMath.h"
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkPNGReader.h"
#include "vtkPNMReader.h"
#include "vtkPVMaterialLibrary.h"
#include "vtkPVRenderView.h"
#include "vtkProcessModule.h"
#include "vtkSMMaterialLibraryProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkTexture.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqLoadMaterialsReaction.h"
#include "pqMaterialAttributesDelegate.h"
#include "pqNewMaterialDialog.h"
#include "pqObjectBuilder.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

#include <QAbstractTableModel>
#include <QMetaProperty>
#include <QSet>
#include <QStandardItemModel>
#include <QVector2D>
#include <QVector3D>
#include <QVector>

#include <array>
#include <sstream>

namespace
{
void AddDefaultValue(
  vtkOSPRayMaterialLibrary* ml, const std::string& matName, const std::string& variable)
{
  const std::string& matType = ml->LookupImplName(matName);
  const auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();
  const auto varType = dic.at(matType).at(variable);
  switch (varType)
  {
    case vtkOSPRayMaterialLibrary::ParameterType::BOOLEAN:
      ml->AddShaderVariable(matName, variable, { 1.0 });
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::FLOAT:
      ml->AddShaderVariable(matName, variable, { 1.5 });
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT:
      ml->AddShaderVariable(matName, variable, { 1.0 });
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::VEC2:
      ml->AddShaderVariable(matName, variable, { 0.0, 0.0 });
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::VEC3:
      ml->AddShaderVariable(matName, variable, { 0.0, 0.0, 0.0 });
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB:
      ml->AddShaderVariable(matName, variable, { 1.0, 1.0, 1.0 });
      break;
    case vtkOSPRayMaterialLibrary::ParameterType::TEXTURE:
      ml->AddTexture(matName, variable, nullptr, "<None>");
      break;
    default:
      vtkGenericWarningMacro("Material property " << variable << " is unsupported.");
      break;
  }
}
}

/**
 * The Qt model associated with the 2D array representation of the material properties
 */
class pqMaterialProxyModel : public QAbstractTableModel
{
public:
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
  void setMaterial(vtkOSPRayMaterialLibrary* ml, const std::string& name)
  {
    this->reset();
    if (!ml)
    {
      return;
    }
    this->MaterialLibrary = ml;
    this->MaterialName = name;
    this->MaterialType = ml->LookupImplName(name);

    const auto& variables = ml->GetDoubleShaderVariableList(name);
    const auto& textures = ml->GetTextureList(name);
    this->beginInsertRows(QModelIndex(), 0, variables.size() + textures.size() - 1);
    for (const auto& var : variables)
    {
      this->VariableNames.push_back(var);
    }
    for (const auto& tex : textures)
    {
      this->VariableNames.push_back(tex);
    }
    this->endInsertRows();
  }

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
   * Returns the number of columns (two in our case)
   */
  int columnCount(const QModelIndex&) const override { return 2; }

  //@{
  /**
   * Query current material informations
   */
  std::string getMaterialType() const { return this->MaterialType; }
  std::string getMaterialName() const { return this->MaterialName; }
  //@}

  /**
   * Return the data at index with role
   */
  QVariant data(const QModelIndex& index, int role) const override;

  /**
   * Overrides the data at index and role with the input variant
   */
  bool setData(const QModelIndex& index, const QVariant& variant, int role) override;

private:
  bool IsSameRole(int role1, pqMaterialEditor::ExtendedItemDataRole role2) const
  {
    return role1 == static_cast<int>(role2);
  }

  vtkOSPRayMaterialLibrary* MaterialLibrary = nullptr;
  std::string MaterialName;
  std::vector<std::string> VariableNames;
  std::string MaterialType;
};

//-----------------------------------------------------------------------------
pqMaterialProxyModel::pqMaterialProxyModel(QObject* p)
  : QAbstractTableModel(p)
{
}

//-----------------------------------------------------------------------------
void pqMaterialProxyModel::reset()
{
  this->beginResetModel();
  this->MaterialLibrary = nullptr;
  this->MaterialType = "";
  this->MaterialName = "";
  this->VariableNames.clear();
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
int pqMaterialProxyModel::rowCount(const QModelIndex& index) const
{
  Q_UNUSED(index);

  if (!this->MaterialLibrary)
  {
    return 0;
  }
  return this->VariableNames.size();
}

//-----------------------------------------------------------------------------
QVariant pqMaterialProxyModel::data(const QModelIndex& index, int role) const
{
  vtkOSPRayMaterialLibrary* ml = this->MaterialLibrary;
  if (!ml || index.row() >= static_cast<long>(this->VariableNames.size()))
  {
    return QVariant();
  }
  const std::string& variableName = this->VariableNames[index.row()];

  if (index.column() == 0)
  {
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
      return QString(variableName.c_str());
    }
    if (this->IsSameRole(role, pqMaterialEditor::ExtendedItemDataRole::PropertyValue))
    {
      return QString(this->MaterialType.c_str());
    }
  }
  else if (index.column() == 1 &&
    this->IsSameRole(role, pqMaterialEditor::ExtendedItemDataRole::PropertyValue))
  {
    const auto& paramDic = vtkOSPRayMaterialLibrary::GetParametersDictionary();
    auto value = ml->GetDoubleShaderVariable(this->MaterialName, variableName);
    switch (paramDic.at(ml->LookupImplName(this->MaterialName)).at(variableName))
    {
      case vtkOSPRayMaterialLibrary::ParameterType::BOOLEAN:
        return static_cast<bool>(value[0]);
      case vtkOSPRayMaterialLibrary::ParameterType::FLOAT:
        return value[0];
      case vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT:
        return value[0];
      case vtkOSPRayMaterialLibrary::ParameterType::VEC2:
      {
        QVector2D vec2;
        vec2.setX(value[0]);
        vec2.setY(value[1]);
        return vec2;
      }
      case vtkOSPRayMaterialLibrary::ParameterType::VEC3:
      {
        QVector3D vec3;
        vec3.setX(value[0]);
        vec3.setY(value[1]);
        vec3.setZ(value[2]);
        return vec3;
      }
      case vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB:
      {
        QColor color;
        color.setRedF(value[0]);
        color.setGreenF(value[1]);
        color.setBlueF(value[2]);
        return color;
      };
      case vtkOSPRayMaterialLibrary::ParameterType::TEXTURE:
      {
        return QString(ml->GetTextureName(this->MaterialName, variableName).c_str());
      };
      default:
        return QVariant();
    }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
bool pqMaterialProxyModel::setData(const QModelIndex& index, const QVariant& variant, int role)
{
  vtkOSPRayMaterialLibrary* ml = this->MaterialLibrary;
  if (!ml || index.row() >= static_cast<long>(this->VariableNames.size()))
  {
    return false;
  }

  if (index.column() == 0 && role == Qt::EditRole)
  {
    int row = index.row();
    ml->RemoveTexture(this->MaterialName, this->VariableNames[row]);
    ml->RemoveShaderVariable(this->MaterialName, this->VariableNames[row]);
    this->VariableNames[row] = variant.toString().toStdString();
    ::AddDefaultValue(ml, this->MaterialName, this->VariableNames[row]);

    QModelIndex sibling = index.sibling(index.row(), 1); // emit for value too
    emit this->dataChanged(index, sibling);
    return true;
  }
  if (index.column() == 1 &&
    this->IsSameRole(role, pqMaterialEditor::ExtendedItemDataRole::PropertyValue))
  {
    std::stringstream ss;
    const std::string& varName = this->VariableNames[index.row()];
    switch (static_cast<QMetaType::Type>(variant.type()))
    {
      case QMetaType::QVector2D:
      {
        QVector2D vec = variant.value<QVector2D>();
        ml->AddShaderVariable(this->MaterialName, varName, { vec.x(), vec.y() });
      }
      break;
      case QMetaType::QVector3D:
      {
        QVector3D vec = variant.value<QVector3D>();
        ml->AddShaderVariable(this->MaterialName, varName, { vec.x(), vec.y(), vec.z() });
      }
      break;
      case QMetaType::QColor:
      {
        QColor col = variant.value<QColor>();
        ml->AddShaderVariable(
          this->MaterialName, varName, { col.redF(), col.greenF(), col.blueF() });
      }
      break;
      case QMetaType::QString:
      {
        QString filePath = variant.value<QString>();
        QFileInfo fileInfo(filePath);
        if (fileInfo.isFile() && fileInfo.isReadable())
        {
          vtkSmartPointer<vtkImageReader2> reader;
          const QString& ext = fileInfo.suffix();
          if (ext == "bmp")
          {
            reader.TakeReference(vtkBMPReader::New());
          }
          else if (ext == "jpg")
          {
            reader.TakeReference(vtkJPEGReader::New());
          }
          else if (ext == "png")
          {
            reader.TakeReference(vtkPNGReader::New());
          }
          else if (ext == "ppm")
          {
            reader.TakeReference(vtkPNMReader::New());
          }
          if (reader)
          {
            reader->SetFileName(filePath.toUtf8().data());
            reader->Update();
            vtkNew<vtkTexture> texture;
            texture->SetInputData(reader->GetOutput());
            texture->Update();
            ml->AddTexture(this->MaterialName, varName, texture, fileInfo.baseName().toStdString());
          }
          else
          {
            ml->AddTexture(this->MaterialName, varName, nullptr, "<None>");
          }
        }
        else
        {
          ml->AddTexture(this->MaterialName, varName, nullptr, "<None>");
        }
      }
      break;
      default:
        ml->AddShaderVariable(this->MaterialName, varName, { variant.toDouble() });
    }

    Q_EMIT this->dataChanged(index, index);
    return true;
  }

  return QAbstractTableModel::setData(index, variant, role);
}

//-----------------------------------------------------------------------------
/**
 * This vtkCommand update the material list when a new file has been loaded.
 */
class vtkNewLibraryLoadedObserver : public vtkCommand
{
public:
  static vtkNewLibraryLoadedObserver* New() { return new vtkNewLibraryLoadedObserver(); }

  void Execute(vtkObject*, unsigned long, void*) override { this->Editor->updateMaterialList(); }

  pqMaterialEditor* Editor;
};

//-----------------------------------------------------------------------------
class pqMaterialEditor::pqInternals
{
public:
  Ui::MaterialEditor Ui;

  pqMaterialProxyModel AttributesModel;
  vtkOSPRayMaterialLibrary* MaterialLibrary = nullptr;

  pqInternals(pqMaterialEditor* self)
  {
    this->Ui.setupUi(self);

    this->Ui.PropertiesView->setModel(&this->AttributesModel);
    this->Ui.PropertiesView->horizontalHeader()->setHighlightSections(false);
    this->Ui.PropertiesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->Ui.PropertiesView->horizontalHeader()->setStretchLastSection(true);
    this->Ui.PropertiesView->verticalHeader()->hide();
    this->Ui.PropertiesView->setItemDelegate(new pqMaterialAttributesDelegate(self));
  }

  ~pqInternals() = default;
};

//-----------------------------------------------------------------------------
pqMaterialEditor::pqMaterialEditor(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqMaterialEditor::pqInternals(this))
{
  // materials
  QObject::connect(
    this->Internals->Ui.AddMaterial, &QPushButton::clicked, this, &pqMaterialEditor::addMaterial);
  QObject::connect(this->Internals->Ui.RemoveMaterial, &QPushButton::clicked, this,
    &pqMaterialEditor::removeMaterial);
  QObject::connect(this->Internals->Ui.LoadMaterials, &QPushButton::clicked, this,
    &pqMaterialEditor::loadMaterials);
  QObject::connect(this->Internals->Ui.AttachMaterial, &QPushButton::clicked, this,
    &pqMaterialEditor::attachMaterial);

  // attributes
  QObject::connect(this->Internals->Ui.SelectMaterial,
    QOverload<int>::of(&QComboBox::currentIndexChanged), this,
    &pqMaterialEditor::updateCurrentMaterialWithIndex);
  QObject::connect(
    this->Internals->Ui.AddProperty, &QPushButton::clicked, this, &pqMaterialEditor::addProperty);
  QObject::connect(this->Internals->Ui.RemoveProperty, &QPushButton::clicked, this,
    &pqMaterialEditor::removeProperty);
  QObject::connect(this->Internals->Ui.DeleteProperties, &QPushButton::clicked, this,
    &pqMaterialEditor::removeAllProperties);

  QObject::connect(&this->Internals->AttributesModel, &QAbstractTableModel::dataChanged, this,
    &pqMaterialEditor::propertyChanged);

  QObject::connect(
    &pqActiveObjects::instance(), &pqActiveObjects::serverChanged, [this](pqServer* server) {
      this->Internals->AttributesModel.reset();
      this->Internals->Ui.SelectMaterial->clear();
      this->Internals->MaterialLibrary = nullptr;
      bool enable = false;
      this->HasWarnedUser = false;
      if (server && !server->isRemote())
      {
        vtkSMProxy* proxy =
          server->proxyManager()->FindProxy("materiallibrary", "materials", "MaterialLibrary");
        if (proxy)
        {
          auto* localObject = proxy->GetClientSideObject();
          if (localObject)
          {
            this->Internals->MaterialLibrary = vtkOSPRayMaterialLibrary::SafeDownCast(
              vtkPVMaterialLibrary::SafeDownCast(localObject)->GetMaterialLibrary());
            if (this->Internals->MaterialLibrary)
            {
              enable = true;
              vtkNew<vtkNewLibraryLoadedObserver> observer;
              observer->Editor = this;
              this->Internals->MaterialLibrary->AddObserver(vtkCommand::UpdateDataEvent, observer);
              this->updateMaterialList();
            }
          }
        }
      }
      if (server && server->isRemote() && this->isVisible())
      {
        vtkGenericWarningMacro(
          "Material Editor disabled because client is connected to a remote server.");
        this->HasWarnedUser = true;
      }
      // Grey out the material editor if CS mode or if we didn't find the ML object
      this->Internals->Ui.AddMaterial->setEnabled(enable);
      this->Internals->Ui.RemoveMaterial->setEnabled(enable);
      this->Internals->Ui.AddProperty->setEnabled(enable);
      this->Internals->Ui.RemoveProperty->setEnabled(enable);
      this->Internals->Ui.PropertiesView->setEnabled(enable);
      this->Internals->Ui.AttachMaterial->setEnabled(enable);
      this->Internals->Ui.SelectMaterial->setEnabled(enable);
      this->Internals->Ui.DeleteProperties->setEnabled(enable);
    });
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
void pqMaterialEditor::addMaterial()
{
  vtkOSPRayMaterialLibrary* ml = this->Internals->MaterialLibrary;
  if (!ml)
  {
    return;
  }

  pqNewMaterialDialog* dialog = new pqNewMaterialDialog(pqCoreUtilities::mainWidget());
  dialog->setWindowTitle("New Material");
  dialog->setMaterialLibrary(ml);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
  QObject::connect(dialog, &pqNewMaterialDialog::accepted, [=]() {
    const std::string matName = dialog->name().toStdString();
    ml->AddMaterial(matName, dialog->type().toUtf8().data());

    // Needed to update vtkSMMaterialDomain instances
    ml->Fire();

    this->Internals->Ui.SelectMaterial->setCurrentText(QString(matName.c_str()));
  });
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::removeMaterial()
{
  vtkOSPRayMaterialLibrary* ml = this->Internals->MaterialLibrary;

  if (ml)
  {
    this->Internals->AttributesModel.reset();
    const auto& removedMaterial = this->currentMaterialName();
    ml->RemoveMaterial(removedMaterial.toStdString());

    // Needed so representations that used this material stop using it
    // We need to do this before firing the material library event because if
    // we are deleting the last material in the list the representation won't update
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
    while ((obj = collection->GetNextItemAsObject()) != nullptr)
    {
      vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(obj);

      if (removedMaterial == vtkSMPropertyHelper(proxy, "OSPRayMaterial").GetAsString())
      {
        vtkSMPropertyHelper(proxy, "OSPRayMaterial").Set("None");
        proxy->UpdateVTKObjects();
      }
    }

    // Needed to update vtkSMMaterialDomain instances
    ml->Fire();
    this->updateCurrentMaterial(this->currentMaterialName().toStdString());
  }
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::attachMaterial()
{
  QString currentMaterial = this->currentMaterialName();
  if (currentMaterial.isEmpty())
  {
    return;
  }

  pqPipelineSource* activeObject = pqActiveObjects::instance().activeSource();
  if (!activeObject)
  {
    vtkGenericWarningMacro("No active source selected.");
    return;
  }

  pqView* activeView = pqActiveObjects::instance().activeView();
  if (!vtkPVRenderView::SafeDownCast(activeView->getClientSideView()))
  {
    vtkGenericWarningMacro("No valid render view selected.");
    return;
  }

  auto* activeRepresentation = activeObject->getRepresentation(activeView);
  if (activeRepresentation)
  {
    vtkSMProxy* representationProxy = activeRepresentation->getProxy();
    if (representationProxy && representationProxy->GetProperty("OSPRayMaterial"))
    {
      vtkSMPropertyHelper(representationProxy, "OSPRayMaterial")
        .Set(currentMaterial.toStdString().c_str());
      representationProxy->UpdateProperty("OSPRayMaterial");
      activeView->render();
    }
    else
    {
      vtkGenericWarningMacro("Current representation is not compatible with OSPRay materials.");
    }
  }
  else
  {
    vtkGenericWarningMacro("No representation for selected source in the selected view.");
  }
}

//-----------------------------------------------------------------------------
std::vector<std::string> pqMaterialEditor::availableParameters()
{
  std::vector<std::string> availableList;

  vtkOSPRayMaterialLibrary* ml = this->Internals->MaterialLibrary;

  const QString& currentText = this->currentMaterialName();
  if (ml && !currentText.isEmpty())
  {
    auto matName = currentText.toStdString();
    auto matType = ml->LookupImplName(matName);
    auto usedVariable = ml->GetDoubleShaderVariableList(matName);
    auto usedTexture = ml->GetTextureList(matName);

    const auto& allParams = vtkOSPRayMaterialLibrary::GetParametersDictionary().at(matType);
    // Filter all available parameters so only the relevant ones are left
    for (const auto& p : allParams)
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
  return availableList;
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::loadMaterials()
{
  pqLoadMaterialsReaction::loadMaterials();
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::addProperty()
{
  auto params = this->availableParameters();
  vtkOSPRayMaterialLibrary* ml = this->Internals->MaterialLibrary;
  if (ml && !params.empty())
  {
    std::string matName = this->currentMaterialName().toStdString();
    ::AddDefaultValue(ml, matName, params[0]);
    this->Internals->AttributesModel.setMaterial(ml, matName);
  }
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::removeProperty()
{
  vtkOSPRayMaterialLibrary* ml = this->Internals->MaterialLibrary;

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

      std::string matName = this->currentMaterialName().toStdString();
      for (const auto& var : selectedVariables)
      {
        // There is no variables with the same name in textures and double variable so
        // we can remove them safely
        ml->RemoveTexture(matName, var.toStdString());
        ml->RemoveShaderVariable(matName, var.toStdString());
      }
      this->updateCurrentMaterial(matName);
    }
  }
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::removeAllProperties()
{
  vtkOSPRayMaterialLibrary* ml = this->Internals->MaterialLibrary;

  if (ml)
  {
    std::string matName = this->currentMaterialName().toStdString();
    ml->RemoveAllShaderVariables(matName);
    ml->RemoveAllTextures(matName);

    this->updateCurrentMaterial(matName);
  }
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::propertyChanged(const QModelIndex& topLeft, const QModelIndex& botRight)
{
  Q_UNUSED(topLeft);
  Q_UNUSED(botRight);

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
  vtkOSPRayMaterialLibrary* ml = this->Internals->MaterialLibrary;

  if (ml)
  {
    this->Internals->Ui.SelectMaterial->clear();

    for (const auto& matName : ml->GetMaterialNames())
    {
      this->Internals->Ui.SelectMaterial->addItem(matName.c_str());
    }
  }
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::updateCurrentMaterialWithIndex(int index)
{
  const QString& label = this->Internals->Ui.SelectMaterial->itemText(index);
  this->updateCurrentMaterial(label.toStdString());
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::updateCurrentMaterial(const std::string& label)
{
  vtkOSPRayMaterialLibrary* ml = this->Internals->MaterialLibrary;
  if (!ml || label.empty())
  {
    return;
  }

  // material type name label update
  std::string materialTypeLabel = "Material Type: ";
  std::string matType = ml->LookupImplName(label);
  if (matType.empty())
  {
    matType = "<none>";
  }
  materialTypeLabel += matType;
  this->Internals->Ui.TypeLabel->setText(materialTypeLabel.c_str());

  this->Internals->AttributesModel.setMaterial(ml, label);
  this->propertyChanged(QModelIndex(), QModelIndex());
}

//-----------------------------------------------------------------------------
void pqMaterialEditor::showEvent(QShowEvent* event)
{
  if (!this->HasWarnedUser && !this->Internals->MaterialLibrary)
  {
    vtkGenericWarningMacro(
      "Material Editor disabled because client is connected to a remote server.");
    this->HasWarnedUser = true;
  }

  this->Superclass::showEvent(event);
}
