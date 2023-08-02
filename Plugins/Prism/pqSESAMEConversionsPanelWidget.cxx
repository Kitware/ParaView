// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSESAMEConversionsPanelWidget.h"
#include "ui_pqSESAMEConversionsPanelWidget.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"

#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLUtilities.h"

#include "vtkPVSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtksys/SystemTools.hxx"

#include <QComboBox>
#include <QLineEdit>
#include <QMetaProperty>
#include <QStyledItemDelegate>

#include <sstream>

//=============================================================================
namespace
{
template <typename T>
T lexical_cast(const std::string& s)
{
  std::stringstream ss(s);

  T result;
  if ((ss >> result).fail() || !(ss >> std::ws).eof())
  {
    throw std::bad_cast();
  }

  return result;
}

//=============================================================================
class SESAMETableConversions
{
public:
  class ConversionVariable
  {
  public:
    ConversionVariable() = default;
    ~ConversionVariable() = default;

    QString Name = "None";
    QString SESAMEUnits = "n/a";
    double SIConversion = 1.0;
    QString SIUnits = "n/a";
    double CGSConversion = 1.0;
    QString CGSUnits = "n/a";
  };

  int TableId = -1;
  QMap<QString, ConversionVariable> VariableConversions;
};

/**
 * The Qt model associated with the 2D array representation of the conversions properties
 */
class pqSESAMEConversionsModel : public QAbstractTableModel
{
public:
  /**
   * Default constructor initialize the Qt hierarchy.
   */
  pqSESAMEConversionsModel(QObject* p = nullptr)
    : QAbstractTableModel(p)
  {
  }

  /**
   * Default destructor for inheritance.
   */
  ~pqSESAMEConversionsModel() override = default;

  /**
   * Sets the conversion information.
   */
  void setConversionInfo(QVector<QString>& variableNames, QVector<QString>& conversionLabels,
    QVector<double>& conversionFactors, bool editableConversionLabel, bool editableFactor)
  {
    this->resetConversionInfo();
    this->EditableConversionLabel = editableConversionLabel;
    this->EditableFactor = editableFactor;
    if (variableNames.size() == conversionLabels.size() &&
      variableNames.size() == conversionFactors.size())
    {
      this->beginInsertRows(QModelIndex(), 0, variableNames.size() - 1);
      this->Conversions.resize(variableNames.size());
      for (int i = 0; i < variableNames.size(); ++i)
      {
        this->Conversions[i].Variable = variableNames[i];
        this->Conversions[i].ConversionLabel = conversionLabels[i];
        this->Conversions[i].Factor = conversionFactors[i];
      }
      this->endInsertRows();
    }
  }

  /**
   * Resets the selection information
   */
  void resetConversionInfo()
  {
    this->beginResetModel();
    this->EditableConversionLabel = false;
    this->EditableFactor = false;
    this->Conversions.clear();
    this->endResetModel();
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
          return tr("Variable");
        case 1:
          return tr("Conversion");
        case 2:
          return tr("Factor");
      }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
  }

  /**
   * Returns the flags associated with this model
   */
  Qt::ItemFlags flags(const QModelIndex& index) const override
  {
    int col = index.column();
    if (col == 0)
    {
      return QAbstractTableModel::flags(index);
    }
    else if (col == 1)
    {
      return this->EditableConversionLabel ? QAbstractTableModel::flags(index) | Qt::ItemIsEditable
                                           : QAbstractTableModel::flags(index);
    }
    else if (col == 2)
    {
      return this->EditableFactor ? QAbstractTableModel::flags(index) | Qt::ItemIsEditable
                                  : QAbstractTableModel::flags(index);
    }
    else
    {
      return QAbstractTableModel::flags(index);
    }
  }

  /**
   * Returns the row count of the 2D array. This corresponds to the number of selections
   * hold by the data producer.
   */
  int rowCount(const QModelIndex&) const override
  {
    return static_cast<int>(this->Conversions.size());
  }

  /**
   * Returns the number of columns (two in our case).
   */
  int columnCount(const QModelIndex&) const override { return 3; }

  /**
   * Returns the data at index with role.
   */
  QVariant data(const QModelIndex& index, int role) const override
  {
    int row = index.row();
    int col = index.column();
    if (this->Conversions.empty() || row >= static_cast<int>(this->Conversions.size()))
    {
      return QVariant();
    }
    if (col == 0)
    {
      if (role == Qt::DisplayRole || role == Qt::EditRole)
      {
        return this->Conversions[row].Variable;
      }
      return QVariant();
    }
    else if (col == 1)
    {
      if (role == Qt::DisplayRole || role == Qt::EditRole)
      {
        return this->Conversions[row].ConversionLabel;
      }
      return QVariant();
    }
    else if (col == 2)
    {
      if (role == Qt::DisplayRole || role == Qt::EditRole)
      {
        return this->Conversions[row].Factor;
      }
      return QVariant();
    }
    return QVariant();
  }

  /**
   * Overrides the data at index and role with the input variant
   */
  bool setData(const QModelIndex& index, const QVariant& variant, int role) override
  {
    int row = index.row();
    int col = index.column();
    if (this->Conversions.empty() || row >= static_cast<int>(this->Conversions.size()))
    {
      return false;
    }
    // col 0 is not editable
    // col 1 is editable but does not need to emit dataChanged because we have a signal that will
    // invoke a setData for col 2
    if (col == 1)
    {
      this->Conversions[row].ConversionLabel = variant.toString();
      return true;
    }
    if (col == 2 && role == Qt::EditRole)
    {
      this->Conversions[row].Factor = variant.toDouble();
      Q_EMIT this->dataChanged(index, index);
      return true;
    }
    return false;
  }

private:
  struct ConversionInfo
  {
    QString Variable;
    QString ConversionLabel;
    double Factor;
  };
  QVector<ConversionInfo> Conversions;
  bool EditableFactor = false;
  bool EditableConversionLabel = false;
};

//=============================================================================
class pqSESAMEConversionsDelegate : public QStyledItemDelegate
{
public:
  pqSESAMEConversionsDelegate(QObject* parent = nullptr)
    : QStyledItemDelegate(parent)
  {
  }
  ~pqSESAMEConversionsDelegate() override = default;

  /**
   * Create the editor for the given index.
   */
  QWidget* createEditor(
    QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
  {
    QComboBox* combo = new QComboBox(parent);

    QStringList paramsList;
    auto conversionPanelWidget = qobject_cast<pqSESAMEConversionsPanelWidget*>(this->parent());
    if (conversionPanelWidget)
    {
      for (auto conversionOption : conversionPanelWidget->getConversionOptions())
      {
        paramsList << conversionOption.first;
      }
    }
    combo->addItems(paramsList);

    QObject::connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
      conversionPanelWidget, &pqSESAMEConversionsPanelWidget::onConversionVariableChanged);

    return combo;
  }

  /**
   * Change the current index of the combo box to the current value of the property.
   */
  void setEditorData(QWidget* editor, const QModelIndex& index) const override
  {
    QString value = index.model()->data(index, Qt::DisplayRole).toString();
    QComboBox* comboBox = qobject_cast<QComboBox*>(editor);
    comboBox->setCurrentIndex(comboBox->findText(value));
  };

  /**
   * Gets data from the editor widget and stores it in the specified model at the item index
   */
  void setModelData(
    QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
  {
    QVariant newValue = editor->property(editor->metaObject()->userProperty().name());
    QComboBox* comboBox = qobject_cast<QComboBox*>(editor);
    QString value = comboBox->currentText();
    model->setData(index, value, Qt::EditRole);
  }

private:
  Q_DISABLE_COPY(pqSESAMEConversionsDelegate)

  QVector<QPair<QString, double>> ConversionOptions;
};
} // namespace

//-----------------------------------------------------------------------------
class pqSESAMEConversionsPanelWidget::pqUI : public Ui::SESAMEConversionsPanelWidget
{
public:
  pqUI(pqSESAMEConversionsPanelWidget* self)
  {
    this->setupUi(self);

    this->PropertiesView->setModel(&this->ConversionsModel);
    this->PropertiesView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->PropertiesView->horizontalHeader()->setStretchLastSection(true);
    this->PropertiesView->setSelectionBehavior(QAbstractItemView::SelectItems);
    this->PropertiesView->setSelectionMode(QAbstractItemView::SingleSelection);
    this->PropertiesView->setItemDelegateForColumn(1, new pqSESAMEConversionsDelegate(self));
    this->PropertiesView->show();
  }

  ~pqUI() = default;

  /**
   * Get the arrays of the table id.
   */
  QVector<QString> getTableArrays()
  {
    if (!this->FlatArraysOfTablesProperty || !this->TableIdProperty)
    {
      qCritical("Missing required properties.");
      return QVector<QString>();
    }

    vtkSMUncheckedPropertyHelper flatArraysOfTablesHelper(this->FlatArraysOfTablesProperty);
    vtkSMUncheckedPropertyHelper tableIdHelper(this->TableIdProperty);

    const int tableId = tableIdHelper.GetAsInt();

    // convert the flat arrays of tables to a map of table id to arrays
    QMap<int, QVector<QString>> arraysOfTables;
    int currentTableId = -1;
    for (unsigned int i = 0; i < flatArraysOfTablesHelper.GetNumberOfElements(); ++i)
    {
      const auto str = flatArraysOfTablesHelper.GetAsString(i);
      try
      {
        int value = lexical_cast<int>(str);
        currentTableId = value;
      }
      catch (const std::bad_cast&)
      {
        if (currentTableId != -1)
        {
          arraysOfTables[currentTableId].push_back(str);
        }
      }
    }
    if (arraysOfTables.find(tableId) != arraysOfTables.end())
    {
      return arraysOfTables[tableId];
    }
    else
    {
      return QVector<QString>();
    }
  }

  /**
   * Get the table id.
   */
  int getTableId()
  {
    if (!this->TableIdProperty)
    {
      qCritical("Missing required properties.");
      return -1;
    }
    vtkSMUncheckedPropertyHelper tableIdHelper(this->TableIdProperty);
    return tableIdHelper.GetAsInt();
  }

  /**
   * Load a conversion file.
   */
  bool loadConversionFile(const QString& filename, vtkTypeUInt32 location)
  {
    this->SESAMEConversions.clear();
    if (filename.isEmpty())
    {
      return false;
    }
    std::string conversionData;
    if (location == vtkPVSession::CLIENT)
    {
      // client could use the same path as the server, but it would not work for Qt registered files
      QFile file(filename);
      // First check to make sure file is valid
      if (!file.open(QFile::ReadOnly))
      {
        qCritical() << "Failed to open file : " << filename;
        return false;
      }
      conversionData = file.readAll().toStdString();
      file.close();
    }
    else // data server
    {
      auto pxm = pqActiveObjects::instance().activeServer()->proxyManager();
      conversionData = pxm->LoadString(filename.toStdString().c_str(), location);
    }

    vtkXMLDataElement* rootElement = vtkXMLUtilities::ReadElementFromString(conversionData.c_str());
    if (!rootElement)
    {
      return false;
    }

    if (strcmp(rootElement->GetName(), "PRISM_Conversions") != 0)
    {
      qCritical() << "Corrupted or Invalid SESAME Conversions File: " << filename;
      return false;
    }

    // Now parse the file
    for (int i = 0; i < rootElement->GetNumberOfNestedElements(); i++)
    {
      vtkXMLDataElement* tableElement = rootElement->GetNestedElement(i);
      QString nameString = tableElement->GetName();

      if (nameString == "Table")
      {
        SESAMETableConversions tableData;

        std::string data_str = tableElement->GetAttribute("Id");
        sscanf(data_str.c_str(), "%d", &tableData.TableId);

        for (int v = 0; v < tableElement->GetNumberOfNestedElements(); v++)
        {
          vtkXMLDataElement* variableElement = tableElement->GetNestedElement(v);
          std::string variableString = variableElement->GetName();
          if (variableString == "Variable")
          {
            SESAMETableConversions::ConversionVariable variableData;
            variableData.Name = variableElement->GetAttribute("Name");

            variableData.SESAMEUnits = variableElement->GetAttribute("SESAME_Units");

            data_str = variableElement->GetAttribute("SESAME_SI");
            sscanf(data_str.c_str(), "%lf", &variableData.SIConversion);
            variableData.SIUnits = variableElement->GetAttribute("SESAME_SI_Units");

            data_str = variableElement->GetAttribute("SESAME_cgs");
            sscanf(data_str.c_str(), "%lf", &variableData.CGSConversion);
            variableData.CGSUnits = variableElement->GetAttribute("SESAME_cgs_Units");

            tableData.VariableConversions.insert(variableData.Name, variableData);
          }
        }
        this->SESAMEConversions.insert(tableData.TableId, tableData);
      }
    }
    rootElement->Delete();

    return true;
  }

  /**
   * Based on the current table id and conversion mode, update the conversion labels and factors.
   */
  void updateConversionLabels()
  {
    auto tableId = this->getTableId();
    QVector<QString> arrays = this->getTableArrays();
    QVector<QString> conversionLabels;
    QVector<double> conversionFactors;
    QVector<QPair<QString, double>> conversionLabelsAndFactorsOptions;

    const auto& tableConversionIter = this->SESAMEConversions.find(tableId);
    if (tableConversionIter != this->SESAMEConversions.end())
    {
      const auto& variableConversions = tableConversionIter.value().VariableConversions;

      // get the conversion labels and factor options
      if (this->ConversionUnits != CUSTOM_UNITS)
      {
        for (const auto& variable : variableConversions)
        {
          QString conversionLabel = variable.Name + " - " + variable.SESAMEUnits;
          double conversionFactor = 1.0;
          if (this->ConversionUnits == SI_UNITS)
          {
            conversionLabel.append(" to " + variable.SIUnits);
            conversionFactor = variable.SIConversion;
          }
          else if (this->ConversionUnits == CGS_UNITS)
          {
            conversionLabel.append(" to " + variable.CGSUnits);
            conversionFactor = variable.CGSConversion;
          }
          conversionLabelsAndFactorsOptions.push_back(
            QPair<QString, double>(conversionLabel, conversionFactor));
        }
      }

      // get the conversion labels and factor for the arrays included in the table
      int counter = 0;
      for (const auto& array : arrays)
      {
        auto variableIter = variableConversions.find(array);
        if (variableIter == variableConversions.end())
        {
          if (array.contains("density", Qt::CaseInsensitive))
          {
            variableIter = variableConversions.find("Density");
          }
          else if (array.contains("temperature", Qt::CaseInsensitive))
          {
            variableIter = variableConversions.find("Temperature");
          }
          else if (array.contains("pressure", Qt::CaseInsensitive))
          {
            variableIter = variableConversions.find("Pressure");
          }
          else if (array.contains("energy", Qt::CaseInsensitive))
          {
            variableIter = variableConversions.find("Energy");
          }
          else if (array.contains("entropy", Qt::CaseInsensitive))
          {
            variableIter = variableConversions.find("Entropy");
          }
          else if (array.contains("speed", Qt::CaseInsensitive))
          {
            variableIter = variableConversions.find("Speed");
          }
        }
        if (variableIter == variableConversions.end())
        {
          variableIter = variableConversions.begin();
        }

        double conversionFactor = 1.0;
        QString conversionLabel;
        if (variableIter != variableConversions.end())
        {
          conversionLabel.append(variableIter.value().Name);
          conversionLabel.append(" - ");
          const auto variableData = variableIter.value();

          switch (this->ConversionUnits)
          {
            case SESAME_UNITS:
            {
              conversionFactor = 1.0;
              conversionLabel.append(variableData.SESAMEUnits);
            }
            break;
            case SI_UNITS:
            {
              conversionFactor = variableData.SIConversion;
              conversionLabel.append(variableData.SESAMEUnits);
              conversionLabel.append(" to ");
              conversionLabel.append(variableData.SIUnits);
            }
            break;
            case CGS_UNITS:
            {
              conversionFactor = variableData.CGSConversion;
              conversionLabel.append(variableData.SESAMEUnits);
              conversionLabel.append(" to ");
              conversionLabel.append(variableData.CGSUnits);
            }
            break;
            case CUSTOM_UNITS:
            {
              QModelIndex index = this->ConversionsModel.index(counter, 2);
              conversionFactor = this->ConversionsModel.data(index, Qt::DisplayRole).toDouble();
              conversionLabel.append(variableData.SESAMEUnits);
            }
          }
        }
        conversionLabels.push_back(conversionLabel);
        conversionFactors.push_back(conversionFactor);
        ++counter;
      }
    }
    else
    {
      for (auto i = 0; i < arrays.size(); ++i)
      {
        conversionLabels.push_back("");
        conversionFactors.push_back(1.0);
      }
    }

    this->CurrentConversionOptions = conversionLabelsAndFactorsOptions;
    this->ConversionsModel.setConversionInfo(arrays, conversionLabels, conversionFactors,
      this->ConversionUnits == SI_UNITS || this->ConversionUnits == CGS_UNITS,
      this->ConversionUnits == CUSTOM_UNITS);
  }

  /**
   * Based on the values of the table, update the conversion factors of the property of the proxy.
   * And return if there is a change in the conversion factors values.
   */
  bool updateConversionsValues()
  {
    vtkSMPropertyHelper valuesHelper(this->VariableConversionValuesProperty);
    const auto numberOfRows =
      static_cast<unsigned int>(this->ConversionsModel.rowCount(QModelIndex()));

    bool changeAvailable = false;
    if (valuesHelper.GetNumberOfElements() != numberOfRows)
    {
      valuesHelper.SetNumberOfElements(numberOfRows);
      changeAvailable = true;
    }
    for (unsigned int i = 0; i < numberOfRows; ++i)
    {
      double factorValue =
        this->ConversionsModel.index(static_cast<int>(i), 2).data(Qt::DisplayRole).toDouble();
      if (factorValue != valuesHelper.GetAsDouble(i))
      {
        valuesHelper.Set(i, factorValue);
        changeAvailable = true;
      }
    }
    return changeAvailable;
  }

  /**
   * Reset conversions factor values.
   * And return if there is a change in the conversion factors.
   */
  bool resetConversionsValues()
  {
    vtkSMPropertyHelper valuesHelper(this->VariableConversionValuesProperty);
    bool changeAvailable = false;
    if (valuesHelper.GetNumberOfElements() != 0)
    {
      valuesHelper.SetNumberOfElements(0);
      changeAvailable = true;
    }
    return changeAvailable;
  }

  enum ConversionUnitsOptions
  {
    SESAME_UNITS = 0,
    SI_UNITS = 1,
    CGS_UNITS = 2,
    CUSTOM_UNITS = 3
  };
  ConversionUnitsOptions ConversionUnits = SI_UNITS;

  QMap<int, SESAMETableConversions> SESAMEConversions;
  QVector<QPair<QString, double>> CurrentConversionOptions;

  pqSESAMEConversionsModel ConversionsModel;

  // we use that to know when the table id changed
  QLineEdit TableIdLabel;

  vtkSMProperty* TableIdProperty = nullptr;
  vtkSMProperty* FlatArraysOfTablesProperty = nullptr;
  vtkSMProperty* VariableConversionValuesProperty = nullptr;
};

//=============================================================================
pqSESAMEConversionsPanelWidget::pqSESAMEConversionsPanelWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , UI(new pqUI(this))
{
  if (auto tableIdProp = smproxy->GetProperty("TableId"))
  {
    this->UI->TableIdProperty = tableIdProp;

    this->addPropertyLink(
      &this->UI->TableIdLabel, "text", SIGNAL(textChanged(const QString&)), tableIdProp);
    QObject::connect(&this->UI->TableIdLabel, &QLineEdit::textChanged, this,
      &pqSESAMEConversionsPanelWidget::onTableIdChanged);
  }
  else
  {
    qCritical("Missing required proxy property TableId");
  }
  if (auto flatArraysOfTablesProp = smgroup->GetProperty("FlatArraysOfTables"))
  {
    this->UI->FlatArraysOfTablesProperty = flatArraysOfTablesProp;
  }
  else
  {
    qCritical("Missing required group property FlatArrayOfTables");
  }
  if (auto variableConversionValues = smgroup->GetProperty("VariableConversionValues"))
  {
    this->UI->VariableConversionValuesProperty = variableConversionValues;
  }
  else
  {
    qCritical("Missing required group property VariableConversionValues");
  }

  // connect restore default conversions file button
  QObject::connect(this->UI->RestoreDefaultConversionsFile, &QToolButton::clicked, this,
    &pqSESAMEConversionsPanelWidget::onRestoreDefaultConversionsFile);

  // connect open conversion file button
  QObject::connect(this->UI->ConversionFileButton, &QToolButton::clicked, this,
    &pqSESAMEConversionsPanelWidget::onConversionFileChanged);

  // connect conversion mode buttons
  QObject::connect(
    this->UI->SESAME, &QRadioButton::clicked, this, &pqSESAMEConversionsPanelWidget::onSESAME);
  QObject::connect(
    this->UI->SI, &QRadioButton::clicked, this, &pqSESAMEConversionsPanelWidget::onSI);
  QObject::connect(
    this->UI->CGS, &QRadioButton::clicked, this, &pqSESAMEConversionsPanelWidget::onCGS);
  QObject::connect(
    this->UI->Custom, &QRadioButton::clicked, this, &pqSESAMEConversionsPanelWidget::onCustom);

  // connect conversion model
  QObject::connect(&this->UI->ConversionsModel, &QAbstractTableModel::dataChanged, this,
    &pqSESAMEConversionsPanelWidget::onTableChanged);

  // get default on conversion radio button
  bool conversionUnitsOn[4] = { this->UI->SESAME->isChecked(), this->UI->SI->isChecked(),
    this->UI->CGS->isChecked(), this->UI->Custom->isChecked() };
  this->UI->ConversionUnits = static_cast<pqUI::ConversionUnitsOptions>(
    std::find(conversionUnitsOn, conversionUnitsOn + 4, true) - conversionUnitsOn);

  // load the default conversions file
  this->onRestoreDefaultConversionsFile();
}

//=============================================================================
pqSESAMEConversionsPanelWidget::~pqSESAMEConversionsPanelWidget()
{
  delete this->UI;
  this->UI = nullptr;
}

//=============================================================================
QVector<QPair<QString, double>> pqSESAMEConversionsPanelWidget::getConversionOptions() const
{
  return this->UI->CurrentConversionOptions;
}

//=============================================================================
void pqSESAMEConversionsPanelWidget::onConversionFileChanged()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  QString filters = tr("Conversion files (*.xml)");
  pqFileDialog fileDialog(server, pqCoreUtilities::mainWidget(), tr("Conversions File"), QString(),
    filters, false, false);
  fileDialog.setObjectName("OpenConversionFileDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    // if a file was selected, load it
    if (!fileDialog.getSelectedFiles().empty())
    {
      auto conversionFilename = fileDialog.getSelectedFiles()[0];
      auto location = fileDialog.getSelectedLocation();
      if (this->UI->loadConversionFile(conversionFilename, location))
      {
        this->UI->updateConversionLabels();
        if (this->UI->updateConversionsValues())
        {
          Q_EMIT this->changeAvailable();
        }
        auto filename = vtksys::SystemTools::GetFilenameName(conversionFilename.toStdString());
        this->UI->ConversionFile->setText(filename.c_str());
      }
      else
      {
        if (this->UI->resetConversionsValues())
        {
          Q_EMIT this->changeAvailable();
        }
        this->UI->ConversionsModel.resetConversionInfo();
        this->UI->ConversionFile->setText("");
      }
    }
  }
}

//=============================================================================
void pqSESAMEConversionsPanelWidget::onRestoreDefaultConversionsFile()
{
  this->UI->ConversionFile->setText(tr("Default"));
  this->UI->loadConversionFile(":/Prism/SESAMEConversions.xml", vtkPVSession::CLIENT);
  this->UI->updateConversionLabels();
  if (this->UI->updateConversionsValues())
  {
    Q_EMIT this->changeAvailable();
  }
}

//=============================================================================
void pqSESAMEConversionsPanelWidget::onTableIdChanged(const QString&)
{
  this->UI->updateConversionLabels();
  if (this->UI->updateConversionsValues())
  {
    Q_EMIT this->changeAvailable();
  }
}

//=============================================================================
void pqSESAMEConversionsPanelWidget::onSESAME()
{
  this->UI->ConversionUnits = pqUI::SESAME_UNITS;
  this->UI->updateConversionLabels();
  if (this->UI->updateConversionsValues())
  {
    Q_EMIT this->changeAvailable();
  }
}

//=============================================================================
void pqSESAMEConversionsPanelWidget::onSI()
{
  this->UI->ConversionUnits = pqUI::SI_UNITS;
  this->UI->updateConversionLabels();
  if (this->UI->updateConversionsValues())
  {
    Q_EMIT this->changeAvailable();
  }
}

//=============================================================================
void pqSESAMEConversionsPanelWidget::onCGS()
{
  this->UI->ConversionUnits = pqUI::CGS_UNITS;
  this->UI->updateConversionLabels();
  if (this->UI->updateConversionsValues())
  {
    Q_EMIT this->changeAvailable();
  }
}

//=============================================================================
void pqSESAMEConversionsPanelWidget::onCustom()
{
  this->UI->ConversionUnits = pqUI::CUSTOM_UNITS;
  this->UI->updateConversionLabels();
  // no need to call updateConversions or emit changeAvailable because the factors don't change
  // we just allow the user to edit the factors
}

//=============================================================================
void pqSESAMEConversionsPanelWidget::onTableChanged(const QModelIndex&, const QModelIndex&)
{
  if (this->UI->updateConversionsValues())
  {
    Q_EMIT this->changeAvailable();
  }
}

//=============================================================================
void pqSESAMEConversionsPanelWidget::onConversionVariableChanged(int index)
{
  int currentRow = this->UI->PropertiesView->currentIndex().row();
  const auto& conversionOptions = this->UI->CurrentConversionOptions;
  if (conversionOptions.size() > index)
  {
    double conversionFactor = conversionOptions[index].second;
    QModelIndex currentFactorIndex = this->UI->ConversionsModel.index(currentRow, 2);
    this->UI->ConversionsModel.setData(currentFactorIndex, conversionFactor, Qt::EditRole);
  }
}
