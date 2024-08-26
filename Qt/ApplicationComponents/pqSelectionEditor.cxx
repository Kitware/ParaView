// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqSelectionEditor.h"
#include "ui_pqSelectionEditor.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqColorChooserButton.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqLiveInsituManager.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqSelectionManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMFieldDataDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMInteractiveSelectionPipeline.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkVector.h"

#include <QAbstractTableModel>
#include <QMessageBox>
#include <QMetaProperty>
#include <QPainter>
#include <QPointer>
#include <QStyledItemDelegate>
#include <QtDebug>

#include <vector>

namespace
{
//-----------------------------------------------------------------------------
QString getTypeFromProxyXMLName(const char* proxyXMLName)
{
  if (strcmp(proxyXMLName, "FrustumSelectionSource") == 0)
  {
    return QString("Frustum Selection");
  }
  else if (strcmp(proxyXMLName, "ValueSelectionSource") == 0)
  {
    return QString("Values Selection");
  }
  else if (strcmp(proxyXMLName, "PedigreeIDSelectionSource") == 0)
  {
    return QString("Pedigree ID Selection");
  }
  else if (strcmp(proxyXMLName, "GlobalIDSelectionSource") == 0)
  {
    return QString("Global ID Selection");
  }
  else if (strcmp(proxyXMLName, "IDSelectionSource") == 0)
  {
    return QString("ID Selection");
  }
  else if (strcmp(proxyXMLName, "CompositeDataIDSelectionSource") == 0)
  {
    return QString("Composite ID Selection");
  }
  else if (strcmp(proxyXMLName, "HierarchicalDataIDSelectionSource") == 0)
  {
    return QString("Hierarchical ID Selection");
  }
  else if (strcmp(proxyXMLName, "LocationSelectionSource") == 0)
  {
    return QString("Location Selection");
  }
  else if (strcmp(proxyXMLName, "BlockSelectionSource") == 0)
  {
    return QString("Block Selection");
  }
  else if (strcmp(proxyXMLName, "BlockSelectorsSelectionSource") == 0)
  {
    return QString("Block Selectors Selection");
  }
  else if (strcmp(proxyXMLName, "ThresholdSelectionSource") == 0)
  {
    return QString("Threshold Selection");
  }
  else if (strcmp(proxyXMLName, "SelectionQuerySource") == 0)
  {
    return QString("Query Selection");
  }
  else
  {
    return QString("Unknown");
  }
}

//=============================================================================
class pqSelectionEditorDeleguate : public QStyledItemDelegate
{
public:
  pqSelectionEditorDeleguate(QObject* parent)
    : QStyledItemDelegate(parent)
  {
  }

  ~pqSelectionEditorDeleguate() override = default;

  void paint(
    QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
  {
    QStyleOptionViewItem modOption = option;

    QColor color = index.model()->data(index, Qt::DisplayRole).value<QColor>();
    // swatch
    int margin = static_cast<int>(0.3 * option.rect.height());
    QRect swatchRect = option.rect.marginsRemoved(QMargins(margin, margin, margin, margin));
    swatchRect.setWidth(swatchRect.height());

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::black);
    painter->setBrush(QBrush(color));
    painter->drawEllipse(swatchRect);
    painter->restore();

    modOption.rect.setLeft(swatchRect.right() + 4);
    modOption.text = QString("(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());

    QStyledItemDelegate::paint(painter, modOption, index);
  }

  void closeCurrentEditor()
  {
    QWidget* editor = qobject_cast<QWidget*>(this->sender());
    Q_EMIT this->commitData(editor);
    Q_EMIT this->closeEditor(editor);
  }

  QWidget* createEditor(
    QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
  {
    QColor color = index.model()->data(index, Qt::DisplayRole).value<QColor>();

    pqColorChooserButton* buttonColor = new pqColorChooserButton(parent);
    buttonColor->setChosenColor(color);
    buttonColor->setText(
      QString("%1, %2, %3").arg(color.red()).arg(color.green()).arg(color.blue()));

    QObject::connect(buttonColor, &pqColorChooserButton::chosenColorChanged, this,
      &pqSelectionEditorDeleguate::closeCurrentEditor);

    return buttonColor;
  }

  void setEditorData(QWidget* editor, const QModelIndex& index) const override
  {
    pqColorChooserButton* colorButton = static_cast<pqColorChooserButton*>(editor);
    QString textColor = index.model()->data(index, Qt::EditRole).toString();
    colorButton->setText(textColor);
    colorButton->update();
  };

  void setModelData(
    QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
  {
    pqColorChooserButton* buttonColor = static_cast<pqColorChooserButton*>(editor);
    QColor color = buttonColor->chosenColor();
    model->setData(index, color, Qt::EditRole);
  }

  void updateEditorGeometry(
    QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const override
  {
    editor->setGeometry(option.rect);
  }
};
} // namespace

/**
 * The Qt model associated with the 2D array representation of the selection properties
 */
class pqSelectionProxyModel : public QAbstractTableModel
{
public:
  /**
   * Default constructor initialize the Qt hierarchy.
   */
  pqSelectionProxyModel(QObject* p = nullptr)
    : QAbstractTableModel(p)
  {
  }

  /**
   * Default destructor for inheritance.
   */
  ~pqSelectionProxyModel() override = default;

  Qt::ItemFlags flags(const QModelIndex& idx) const override
  {
    // Color column should be editable to changes the color of the selected selection
    if (idx.column() == 2)
    {
      // Color the selection isn't supported on macos arm, for more details:
      // https://gitlab.kitware.com/paraview/paraview/-/issues/21786
#if defined(__APPLE__) && defined(__arm64__)
      return Qt::NoItemFlags;
#else
      return QAbstractTableModel::flags(idx) | Qt::ItemFlag::ItemIsEditable;
#endif
    }
    return QAbstractTableModel::flags(idx);
  }

  /**
   * Sets the selection information.
   */
  void setSelectionInfo(
    vtkSMInputProperty* selectionInputs, vtkSMStringVectorProperty* selectionNames)
  {
    this->resetSelectionInfo();
    if (!selectionInputs || selectionInputs->GetNumberOfProxies() == 0 || !selectionNames ||
      selectionNames->GetNumberOfElements() == 0)
    {
      return;
    }

    this->beginInsertRows(QModelIndex(), 0, selectionInputs->GetNumberOfProxies() - 1);
    this->SavedSelections.resize(selectionInputs->GetNumberOfProxies());
    this->SavedColors.resize(selectionInputs->GetNumberOfProxies(), this->DefaultSelectionColor);

    for (unsigned int i = 0; i < selectionInputs->GetNumberOfProxies(); ++i)
    {
      this->SavedSelections[i].Type =
        getTypeFromProxyXMLName(selectionInputs->GetProxy(i)->GetXMLName());
      this->SavedSelections[i].Name = selectionNames->GetElement(i);
    }
    this->endInsertRows();
  }

  /**
   * Resets the selection information
   */
  void resetSelectionInfo()
  {
    this->beginResetModel();
    this->SavedSelections.clear();
    this->endResetModel();
  }

  /**
   * Resets the colors saved
   */
  void resetColorsInfo()
  {
    this->beginResetModel();
    this->SavedColors.clear();
    this->endResetModel();
  }

  /**
   * Set the default color used for the selection color
   */
  void SetDefaultColor(double* color)
  {
    if (!color)
    {
      return;
    }
    this->DefaultSelectionColor = QColor(color[0] * 255, color[1] * 255, color[2] * 255);
  }

  /**
   * Return the color saved at the given index to fit with vtkAppendSelection.
   */
  std::vector<double> GetColor(unsigned int idx)
  {
    std::vector<double> color;
    color.resize(3);
    if (idx >= this->SavedColors.size())
    {
      return color;
    }

    QColor qColor = this->SavedColors[idx];
    color[0] = qColor.redF();
    color[1] = qColor.greenF();
    color[2] = qColor.blueF();

    return color;
  }

  void RemoveColorAt(unsigned int idx)
  {
    if (idx >= this->SavedColors.size())
    {
      return;
    }

    this->SavedColors.erase(
      std::remove(this->SavedColors.begin(), this->SavedColors.end(), this->SavedColors[idx]),
      this->SavedColors.end());
  }
  //-----------------------------------------------------------------------------
  // Qt functions overrides
  //-----------------------------------------------------------------------------

  /**
   * Construct the header data for this model.
   */
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override
  {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
      switch (section)
      {
        case 0:
          return "Name";
        case 1:
          return "Type";
        case 2:
          return "Color";
      }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
  }

  /**
   * Returns the row count of the 2D array. This corresponds to the number of selections
   * hold by the data producer.
   */
  int rowCount(const QModelIndex&) const override
  {
    return static_cast<int>(this->SavedSelections.size());
  }

  /**
   * Returns the number of columns (three in our case).
   */
  int columnCount(const QModelIndex&) const override { return 3; }

  /**
   * Returns the data at index with role.
   */
  QVariant data(const QModelIndex& index, int role) const override
  {
    int row = index.row();
    int col = index.column();
    if (this->SavedSelections.empty() || row >= static_cast<int>(this->SavedSelections.size()))
    {
      return false;
    }
    if (col == 0)
    {
      if (role == Qt::DisplayRole || role == Qt::EditRole)
      {
        return this->SavedSelections[row].Name;
      }
      return QVariant();
    }
    else if (col == 1)
    {
      if (role == Qt::DisplayRole || role == Qt::EditRole)
      {
        return this->SavedSelections[row].Type;
      }
      return QVariant();
    }
    else if (col == 2)
    {
      if (role == Qt::DisplayRole || role == Qt::EditRole)
      {
        return this->SavedColors[row];
      }
      return QVariant();
    }
    return QVariant();
  }

  bool setData(const QModelIndex& index, const QVariant& variant, int role) override
  {
    if (!index.isValid())
    {
      return false;
    }

    const int colorColumn = 2;
    if (index.column() == colorColumn && index.row() < static_cast<int>(this->SavedColors.size()))
    {
      QColor c = variant.value<QColor>();
      SavedColors[index.row()] = c;
    }

    return QAbstractTableModel::setData(index, variant, role);
  }

  /**
   * Return true if there is at least one frustrum selection saved in the model.
   */
  bool hasFrustrumSelection()
  {
    for (std::size_t i = 0; i < this->SavedSelections.size(); i++)
    {
      if (this->SavedSelections[i].Type == "Frustum Selection")
      {
        return true;
      }
    }

    return false;
  }

private:
  struct SelectionInfo
  {
    QString Name;
    QString Type;
  };
  std::vector<SelectionInfo> SavedSelections;
  std::vector<QColor> SavedColors;
  QColor DefaultSelectionColor;
};

//-----------------------------------------------------------------------------
class pqSelectionEditor::pqInternal : public Ui::SelectionEditor
{
public:
  pqInternal(pqSelectionEditor* self)
  {
    this->setupUi(self);

    this->PropertiesView->setModel(&this->SelectionModel);
    this->PropertiesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->PropertiesView->horizontalHeader()->setStretchLastSection(true);
    this->PropertiesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->PropertiesView->setSelectionMode(QAbstractItemView::SingleSelection);
    this->PropertiesView->setItemDelegateForColumn(2, new pqSelectionEditorDeleguate(self));
    this->PropertiesView->show();

    // source
    this->SourceInfo->setEnabled(false);
    this->SourceInfo->setText(QString("(%1)").arg(tr("none")));

    // element type
    this->ElementTypeInfo->setEnabled(false);
    this->setElementType(this->getSelectionInputElementType());

    // disable buttons and expression
    this->Expression->setEnabled(false);
    this->AddActiveSelection->setEnabled(false);
    this->RemoveSelectedSelection->setEnabled(false);
    this->RemoveAllSelections->setEnabled(false);
    this->ActivateCombinedSelections->setEnabled(false);
  }

  ~pqInternal() = default;

  /**
   * Gets the element type of the saved port's selection or vtkDataObject::POINT.
   */
  int getSelectionInputElementType()
  {
    if (!this->SavedPort)
    {
      return vtkDataObject::POINT;
    }
    auto activeAppendSelections = this->SavedPort->getSelectionInput();
    if (!activeAppendSelections ||
      vtkSMPropertyHelper(activeAppendSelections, "Input").GetNumberOfElements() == 0)
    {
      return vtkDataObject::POINT;
    }
    // Checking only one of the inputs is sufficient.
    vtkSMProxy* firstSelectionSourceAS =
      vtkSMPropertyHelper(activeAppendSelections, "Input").GetAsProxy(0);
    if (firstSelectionSourceAS->GetProperty("ElementType") != nullptr)
    {
      return vtkSMPropertyHelper(firstSelectionSourceAS, "ElementType").GetAsInt();
    }
    else
    {
      return vtkSelectionNode::ConvertSelectionFieldToAttributeType(
        vtkSMPropertyHelper(firstSelectionSourceAS, "FieldType").GetAsInt());
    }
  }

  /**
   * Sets the element type
   */
  void setElementType(int elementType)
  {
    static const QString imageStyle = "width=\"16\" height=\"16\"";
    if (this->ElementType == elementType)
    {
      return;
    }
    this->ElementType = elementType;
    QString elementTypeName = vtkSMFieldDataDomain::GetElementTypeAsString(elementType);
    QString imageName;
    switch (elementType)
    {
      default:
      case vtkDataObject::POINT:
        imageName = "<img src=\":/pqWidgets/Icons/pqPointData.svg\" " + imageStyle + ">";
        break;
      case vtkDataObject::CELL:
        imageName = "<img src=\":/pqWidgets/Icons/pqCellData.svg\" " + imageStyle + ">";
        break;
      case vtkDataObject::FIELD:
        imageName = "<img src=\":/pqWidgets/Icons/pqGlobalData.svg\" " + imageStyle + ">";
        break;
      case vtkDataObject::VERTEX:
        imageName = "<img src=\":/pqWidgets/Icons/pqPointData.svg\" " + imageStyle + ">";
        break;
      case vtkDataObject::EDGE:
        imageName = "<img src=\":/pqWidgets/Icons/pqEdgeCenterData.svg\" " + imageStyle + ">";
        break;
      case vtkDataObject::ROW:
        imageName = "<img src=\":/pqWidgets/Icons/pqSpreadsheet.svg\" " + imageStyle + ">";
        break;
    }
    this->ElementTypeInfo->setText(imageName + " " + elementTypeName);
  }

  int ElementType = -1; // This value is assigned to remove a compiler warning.
  pqSelectionProxyModel SelectionModel;
  QPointer<pqServer> Server;
  QPointer<pqOutputPort> SavedPort;
  vtkSmartPointer<vtkSMSourceProxy> SavedAppendSelections;
  vtkWeakPointer<vtkSMRenderViewProxy> InteractiveRenderView;
  QMetaObject::Connection SourceVisibilityChangedConnection;
};

//-----------------------------------------------------------------------------
pqSelectionEditor::pqSelectionEditor(QWidget* parent)
  : Superclass(parent)
  , Internal(new pqInternal(this))
{
  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::serverChanged, this,
    &pqSelectionEditor::onActiveServerChanged);
  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::viewChanged, this,
    &pqSelectionEditor::onActiveViewChanged);
  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::sourceChanged, this,
    &pqSelectionEditor::onActiveSourceChanged);

  auto serverManagerModel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(serverManagerModel, &pqServerManagerModel::sourceRemoved, this,
    &pqSelectionEditor::onAboutToRemoveSource);

  auto selManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SELECTION_MANAGER"));
  if (selManager)
  {
    QObject::connect(selManager, &pqSelectionManager::selectionChanged, this,
      &pqSelectionEditor::onSelectionChanged);
  }

  QObject::connect(this->Internal->Expression, &QLineEdit::textChanged, this,
    &pqSelectionEditor::onExpressionChanged);
  QObject::connect(this->Internal->PropertiesView->selectionModel(),
    &QItemSelectionModel::selectionChanged, this, &pqSelectionEditor::onTableSelectionChanged);
  QObject::connect(this->Internal->AddActiveSelection, &QPushButton::clicked, this,
    &pqSelectionEditor::onAddActiveSelection);
  QObject::connect(this->Internal->RemoveSelectedSelection, &QPushButton::clicked, this,
    &pqSelectionEditor::onRemoveSelectedSelection);
  QObject::connect(this->Internal->RemoveAllSelections, &QPushButton::clicked, this,
    &pqSelectionEditor::onRemoveAllSelections);
  QObject::connect(this->Internal->ActivateCombinedSelections, &QPushButton::clicked, this,
    &pqSelectionEditor::onActivateCombinedSelections);
}

//-----------------------------------------------------------------------------
pqSelectionEditor::~pqSelectionEditor()
{
  delete this->Internal;
  this->Internal = nullptr;
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::onActiveServerChanged(pqServer* server)
{
  if (!server)
  {
    return;
  }

  if (pqLiveInsituManager::isInsituServer(server))
  {
    // it doesn't make sense for the Selection Editor to handle catalyst server as we cannot
    // perform a selection on this server.
    return;
  }

  if (server == this->Internal->Server)
  {
    return;
  }
  this->Internal->Server = server;

  // From the server, find the default selection color in paraview settings
  double color[3] = { 0, 0, 0 };
  vtkSMProxy* colorPalette =
    this->Internal->Server->proxyManager()->GetProxy("settings", "ColorPalette");
  vtkSMPropertyHelper(colorPalette, "SelectionColor").Get(color, 3);
  this->Internal->SelectionModel.SetDefaultColor(color);

  // initialize saved append selections
  auto pxm = server->proxyManager();
  this->Internal->SavedAppendSelections.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "AppendSelections")));
  this->removeAllSelections(vtkDataObject::POINT);
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::clearInteractiveSelection()
{
  if (this->Internal->PropertiesView->selectionModel()->hasSelection())
  {
    this->Internal->PropertiesView->selectionModel()->clearSelection();
    this->hideInteractiveSelection();
  }
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::onActiveViewChanged(pqView*)
{
  this->clearInteractiveSelection();
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::onAboutToRemoveSource(pqPipelineSource* source)
{
  // onAboutToRemoveSource is always called after onActiveSourceChanged
  // Since onSelectionChanged is the only function that changes explicitly the saved port to
  // something that is not nullptr, if the source of saved port is to be removed, we reset
  // name and saved selections.
  if (this->Internal->SavedPort && source == this->Internal->SavedPort->getSource())
  {
    // set name to none and remove selections because the saved source/port will be removed
    if (this->Internal->SourceInfo->isEnabled())
    {
      this->Internal->SourceInfo->setEnabled(false);
      this->Internal->SourceInfo->setText(QString("(%1)").arg(tr("none")));
      this->removeAllSelections(vtkDataObject::POINT);
    }
  }
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::onActiveSourceChanged(pqPipelineSource* source)
{
  if (this->Internal->SourceVisibilityChangedConnection)
  {
    // Make sure we have only one visibility changed qt connection
    QObject::disconnect(this->Internal->SourceVisibilityChangedConnection);
  }

  if (source)
  {
    // check for visibility changes
    this->Internal->SourceVisibilityChangedConnection = QObject::connect(
      source, &pqPipelineSource::visibilityChanged, this, &pqSelectionEditor::onVisibilityChanged);
  }
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::onVisibilityChanged(pqPipelineSource* source, pqDataRepresentation* repr)
{
  // if the changed source is the same as the saved source
  if (this->Internal->SavedPort && this->Internal->SavedPort->getSource() == source)
  {
    // if the representation is no longer visible
    if (repr && !repr->isVisible())
    {
      this->clearInteractiveSelection();
    }
  }
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::onSelectionChanged(pqOutputPort* selectionPort)
{
  // The saved port will be the same as the selectionPort unless selection port becomes nullptr.
  // nullptr means that the selection is cleared. but the port might still be valid.
  // nullptr cases are handled by onAboutToRemoveSource
  if (selectionPort && selectionPort != this->Internal->SavedPort)
  {
    this->Internal->SavedPort = selectionPort;
    this->Internal->SourceInfo->setText(this->Internal->SavedPort->prettyName());
    if (!this->Internal->SourceInfo->isEnabled())
    {
      this->Internal->SourceInfo->setEnabled(true);
    }
    this->removeAllSelections(this->Internal->getSelectionInputElementType());
  }
  vtkSMPropertyHelper selectionInputs(this->Internal->SavedAppendSelections, "Input");
  if (selectionInputs.GetNumberOfElements() == 0)
  {
    this->Internal->setElementType(this->Internal->getSelectionInputElementType());
  }
  this->Internal->AddActiveSelection->setEnabled(
    this->Internal->SavedPort && this->Internal->SavedPort->getSelectionInput());
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::onExpressionChanged(const QString& string)
{
  vtkSMPropertyHelper(this->Internal->SavedAppendSelections, "Expression")
    .Set(string.toUtf8().data());
  this->Internal->SavedAppendSelections->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::onTableSelectionChanged(
  const QItemSelection& selected, const QItemSelection& /*deselected*/)
{
  if (!selected.indexes().empty())
  {
    // only one row can be selected
    auto selectedRow = static_cast<unsigned int>(selected.indexes()[0].row());
    this->showInteractiveSelection(selectedRow);
  }
  else
  {
    this->hideInteractiveSelection();
  }
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::showInteractiveSelection(unsigned int selectedRow)
{
  if (!this->Internal->RemoveSelectedSelection->isEnabled())
  {
    this->Internal->RemoveSelectedSelection->setEnabled(true);
  }
  // if the active view is a render view
  this->Internal->InteractiveRenderView = nullptr;
  if (auto activeView = pqActiveObjects::instance().activeView())
  {
    this->Internal->InteractiveRenderView =
      vtkSMRenderViewProxy::SafeDownCast(activeView->getViewProxy());
    // and the source has a representation
    vtkSMSourceProxy* repr = nullptr;
    if (this->Internal->SavedPort && this->Internal->InteractiveRenderView)
    {
      pqDataRepresentation* pqRepr = this->Internal->SavedPort->getRepresentation(activeView);
      // that is visible
      if (pqRepr && pqRepr->isVisible())
      {
        repr = vtkSMSourceProxy::SafeDownCast(pqRepr->getProxy());
        if (repr)
        {
          BEGIN_UNDO_EXCLUDE();
          // create an append selections proxy using the selected selection
          vtkSmartPointer<vtkSMSourceProxy> interactiveSelectionSource;
          interactiveSelectionSource.TakeReference(vtkSMSourceProxy::SafeDownCast(
            vtkSMSelectionHelper::NewAppendSelectionsFromSelectionSource(
              vtkSMSourceProxy::SafeDownCast(
                vtkSMPropertyHelper(this->Internal->SavedAppendSelections, "Input")
                  .GetAsProxy(selectedRow)))));
          // show the selected selection in the render view
          auto pipeline = vtkSMInteractiveSelectionPipeline::GetInstance();
          pipeline->Show(repr, interactiveSelectionSource, this->Internal->InteractiveRenderView);
          END_UNDO_EXCLUDE();
        }
      }
      else
      {
        this->Internal->InteractiveRenderView = nullptr;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::hideInteractiveSelection()
{
  if (this->Internal->RemoveSelectedSelection->isEnabled())
  {
    this->Internal->RemoveSelectedSelection->setEnabled(false);
  }
  if (this->Internal->InteractiveRenderView)
  {
    // deselect the selection
    auto pipeline = vtkSMInteractiveSelectionPipeline::GetInstance();
    pipeline->Hide(this->Internal->InteractiveRenderView);
    // reset InteractiveRenderView
    this->Internal->InteractiveRenderView = nullptr;
  }
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::onAddActiveSelection()
{
  if (!this->Internal->SavedPort)
  {
    return;
  }
  auto activeAppendSelections = this->Internal->SavedPort->getSelectionInput();
  // check element type and activeAppendSelection's element type
  int activeAppendSelectionsElementType = this->Internal->getSelectionInputElementType();
  if (this->Internal->ElementType != activeAppendSelectionsElementType)
  {
    auto userAnswer =
      QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Different Element Type"),
        tr("The current active selection has a different "
           "element type compared to chosen element type.\n"
           "Are you sure you want to continue?"),
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
    // don't do anything if user cancels
    if (userAnswer == QMessageBox::Cancel)
    {
      return;
    }
    else
    {
      // change the element type
      this->Internal->setElementType(activeAppendSelectionsElementType);
    }
  }
  if (!this->Internal->ElementTypeInfo->isEnabled())
  {
    this->Internal->ElementTypeInfo->setEnabled(true);
  }
  // create a copy of the selection input which is an append selections
  auto pxm = this->Internal->Server->proxyManager();
  vtkSmartPointer<vtkSMSourceProxy> activeAppendSelectionsCopy;
  activeAppendSelectionsCopy.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "AppendSelections")));
  activeAppendSelectionsCopy->Copy(activeAppendSelections);

  // perform the addition of the selection
  vtkSMSelectionHelper::AddSelection(
    this->Internal->SavedAppendSelections, activeAppendSelectionsCopy, /*deep copy*/ true);

  // copy results back to saved append selections
  this->Internal->SavedAppendSelections->Copy(activeAppendSelectionsCopy);
  this->Internal->SavedAppendSelections->UpdateVTKObjects();

  // update table information
  this->Internal->SelectionModel.setSelectionInfo(
    vtkSMInputProperty::SafeDownCast(this->Internal->SavedAppendSelections->GetProperty("Input")),
    vtkSMStringVectorProperty::SafeDownCast(
      this->Internal->SavedAppendSelections->GetProperty("SelectionNames")));

  this->Internal->Expression->setText(
    vtkSMPropertyHelper(this->Internal->SavedAppendSelections, "Expression").GetAsString());
  if (!this->Internal->Expression->isEnabled())
  {
    this->Internal->Expression->setEnabled(true);
  }
  if (this->Internal->AddActiveSelection->isEnabled())
  {
    this->Internal->AddActiveSelection->setEnabled(false);
  }
  if (!this->Internal->RemoveAllSelections->isEnabled())
  {
    this->Internal->RemoveAllSelections->setEnabled(true);
  }
  if (!this->Internal->ActivateCombinedSelections->isEnabled())
  {
    this->Internal->ActivateCombinedSelections->setEnabled(true);
  }
  // since the selection table changes the interactive selection needs to be cleared as well
  this->clearInteractiveSelection();
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::onRemoveSelectedSelection()
{
  auto selectionModel = this->Internal->PropertiesView->selectionModel();
  if (selectionModel->hasSelection())
  {
    // only one row can be selected
    auto selectedRow = static_cast<unsigned int>(selectionModel->selectedRows()[0].row());
    vtkSMPropertyHelper savedSelectionInputs(this->Internal->SavedAppendSelections, "Input");
    vtkSMPropertyHelper savedSelectionNames(
      this->Internal->SavedAppendSelections, "SelectionNames");

    // remove the selected selection
    unsigned int numInputs = savedSelectionInputs.GetNumberOfElements();
    for (unsigned int i = selectedRow + 1; i < numInputs; ++i)
    {
      savedSelectionInputs.Set(i - 1, savedSelectionInputs.GetAsProxy(i));
      savedSelectionNames.Set(i - 1, savedSelectionNames.GetAsString(i));
    }
    savedSelectionInputs.SetNumberOfElements(savedSelectionInputs.GetNumberOfElements() - 1);
    savedSelectionNames.SetNumberOfElements(savedSelectionNames.GetNumberOfElements() - 1);
    this->Internal->SavedAppendSelections->UpdateVTKObjects();

    this->Internal->SelectionModel.RemoveColorAt(selectedRow);

    if (savedSelectionInputs.GetNumberOfElements() == 0)
    {
      this->removeAllSelections(this->Internal->getSelectionInputElementType());
    }
    else
    {
      this->clearInteractiveSelection();
      // update table information
      this->Internal->SelectionModel.setSelectionInfo(
        vtkSMInputProperty::SafeDownCast(
          this->Internal->SavedAppendSelections->GetProperty("Input")),
        vtkSMStringVectorProperty::SafeDownCast(
          this->Internal->SavedAppendSelections->GetProperty("SelectionNames")));
    }
  }
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::onRemoveAllSelections()
{
  this->removeAllSelections(this->Internal->getSelectionInputElementType());
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::removeAllSelections(int elementType)
{
  this->clearInteractiveSelection();
  // update table information
  this->Internal->SavedAppendSelections->ResetPropertiesToDefault(vtkSMProxy::ONLY_XML);
  this->Internal->SelectionModel.resetSelectionInfo();
  this->Internal->SelectionModel.resetColorsInfo();
  // update the GUI
  this->Internal->ElementTypeInfo->setEnabled(false);
  this->Internal->setElementType(elementType);
  if (this->Internal->SourceInfo->isEnabled())
  {
    this->Internal->AddActiveSelection->setEnabled(
      this->Internal->SavedPort && this->Internal->SavedPort->getSelectionInput());
  }
  this->Internal->RemoveAllSelections->setEnabled(false);
  this->Internal->Expression->setEnabled(false);
  this->Internal->Expression->setText("");
  this->Internal->ActivateCombinedSelections->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqSelectionEditor::onActivateCombinedSelections()
{
  if (!this->Internal->SavedPort)
  {
    return;
  }
  // deep copy savedAppendSelections before setting as activeAppendSelections to avoid issues when
  // either the activeAppendSelections or the savedAppendSelections are cleared
  auto pxm = this->Internal->Server->proxyManager();
  auto& savedAppendSelections = this->Internal->SavedAppendSelections;
  vtkSmartPointer<vtkSMSourceProxy> savedAppendSelectionsCopy;
  savedAppendSelectionsCopy.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "AppendSelections")));
  unsigned int numInputs =
    vtkSMPropertyHelper(savedAppendSelections, "Input").GetNumberOfElements();

  std::vector<vtkVector3d> colors;
  colors.resize(numInputs);
  for (unsigned int i = 0; i < numInputs; ++i)
  {
    auto selectionSource = vtkSMPropertyHelper(savedAppendSelections, "Input").GetAsProxy(i);
    vtkSmartPointer<vtkSMSourceProxy> selectionSourceCopy;
    selectionSourceCopy.TakeReference(
      vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", selectionSource->GetXMLName())));
    selectionSourceCopy->Copy(selectionSource);
    selectionSourceCopy->UpdateVTKObjects();
    vtkSMPropertyHelper(savedAppendSelectionsCopy, "Input").Add(selectionSourceCopy);

    auto color = this->Internal->SelectionModel.GetColor(i);
    colors[i][0] = color[0];
    colors[i][1] = color[1];
    colors[i][2] = color[2];
    vtkSMPropertyHelper(savedAppendSelectionsCopy, "SelectionNames")
      .Set(i, vtkSMPropertyHelper(savedAppendSelections, "SelectionNames").GetAsString(i));
  }

  // Frustrum Selection doesn't support the color selection
  if (!this->Internal->SelectionModel.hasFrustrumSelection())
  {
    vtkSMPropertyHelper(savedAppendSelectionsCopy, "SelectionColors")
      .Set(&colors[0][0], numInputs * 3);
  }

  vtkSMPropertyHelper(savedAppendSelectionsCopy, "Expression")
    .Set(vtkSMPropertyHelper(savedAppendSelections, "Expression").GetAsString());
  vtkSMPropertyHelper(savedAppendSelectionsCopy, "InsideOut")
    .Copy(vtkSMPropertyHelper(savedAppendSelections, "InsideOut"));
  savedAppendSelectionsCopy->UpdateVTKObjects();
  savedAppendSelectionsCopy->UpdatePipeline();

  // ugliness with selection manager -- need a better way of doing this!
  auto selectionManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SELECTION_MANAGER"));
  if (selectionManager)
  {
    auto serverManagerModel = pqApplicationCore::instance()->getServerManagerModel();
    auto pipelineProxy =
      serverManagerModel->findItem<pqPipelineSource*>(this->Internal->SavedPort->getSourceProxy());
    Q_ASSERT(pipelineProxy);
    selectionManager->select(
      pipelineProxy->getOutputPort(this->Internal->SavedPort->getPortNumber()));
  }

  // set activeAppendSelections
  this->clearInteractiveSelection();
  this->Internal->SavedPort->setSelectionInput(savedAppendSelectionsCopy, 0);
}
