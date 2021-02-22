#include "pqCustomViewpointButtonDialog.h"
#include "ui_pqCustomViewpointButtonDialog.h"

#include <QDebug>
#include <QToolButton>

#include "pqFileDialog.h"
#include "vtkIndent.h"
#include "vtkSMCameraConfigurationFileInfo.h"

#include <cassert>
#include <sstream>
#include <string>
#include <vtk_pugixml.h>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#define QT_ENDL endl
#else
#define QT_ENDL Qt::endl
#endif

#define pqErrorMacro(estr)                                                                         \
  qDebug() << "Error in:" << QT_ENDL << __FILE__ << ", line " << __LINE__ << QT_ENDL << "" estr    \
           << QT_ENDL;

// User interface
//=============================================================================
class pqCustomViewpointButtonDialogUI : public Ui::pqCustomViewpointButtonDialog
{
  struct RowData
  {
    QPointer<QLabel> IndexLabel;
    QPointer<QLineEdit> ToolTipEdit;
    QPointer<QPushButton> AssignButton;
    QPointer<QToolButton> DeleteButton;
  };

  QPointer< ::pqCustomViewpointButtonDialog> Parent;
  QList<RowData> Rows;

public:
  pqCustomViewpointButtonDialogUI(::pqCustomViewpointButtonDialog* parent)
    : Parent(parent)
  {
  }
  ~pqCustomViewpointButtonDialogUI() { this->setNumberOfRows(0); }

  void setNumberOfRows(int rows)
  {
    if (this->Rows.size() == rows)
    {
      return;
    }

    // enable/disable add button.
    this->add->setEnabled(rows < ::pqCustomViewpointButtonDialog::MAXIMUM_NUMBER_OF_ITEMS);

    // remove extra rows.
    for (int cc = this->Rows.size() - 1; cc >= rows; --cc)
    {
      auto& arow = this->Rows[cc];
      this->gridLayout->removeWidget(arow.IndexLabel);
      this->gridLayout->removeWidget(arow.ToolTipEdit);
      this->gridLayout->removeWidget(arow.AssignButton);
      if (arow.DeleteButton)
      {
        this->gridLayout->removeWidget(arow.DeleteButton);
      }
      delete arow.IndexLabel;
      delete arow.ToolTipEdit;
      delete arow.AssignButton;
      delete arow.DeleteButton;
      this->Rows.pop_back();
    }

    // add new rows.
    for (int cc = this->Rows.size(); cc < rows; ++cc)
    {
      RowData arow;
      arow.IndexLabel = new QLabel(QString::number(cc + 1), this->Parent);
      arow.IndexLabel->setAlignment(Qt::AlignCenter);
      arow.ToolTipEdit = new QLineEdit(this->Parent);
      arow.ToolTipEdit->setToolTip("This text will be set to the buttons tool tip.");
      arow.ToolTipEdit->setText(::pqCustomViewpointButtonDialog::DEFAULT_TOOLTIP);
      arow.ToolTipEdit->setObjectName(QString("toolTip%1").arg(cc));
      arow.AssignButton = new QPushButton("Current Viewpoint", this->Parent);
      arow.AssignButton->setProperty("pqCustomViewpointButtonDialog_INDEX", cc);
      arow.AssignButton->setObjectName(QString("currentViewpoint%1").arg(cc));
      this->Parent->connect(arow.AssignButton, SIGNAL(clicked()), SLOT(assignCurrentViewpoint()));
      this->gridLayout->addWidget(arow.IndexLabel, cc + 1, 0);
      this->gridLayout->addWidget(arow.ToolTipEdit, cc + 1, 1);
      this->gridLayout->addWidget(arow.AssignButton, cc + 1, 2);
      if (cc >= ::pqCustomViewpointButtonDialog::MINIMUM_NUMBER_OF_ITEMS)
      {
        arow.DeleteButton = new QToolButton(this->Parent);
        arow.DeleteButton->setObjectName(QString("delete%1").arg(cc));
        arow.DeleteButton->setIcon(QIcon(":/QtWidgets/Icons/pqDelete.svg"));
        arow.DeleteButton->setProperty("pqCustomViewpointButtonDialog_INDEX", cc);
        this->gridLayout->addWidget(arow.DeleteButton, cc + 1, 3);
        this->Parent->connect(arow.DeleteButton, SIGNAL(clicked()), SLOT(deleteRow()));
      }
      this->Rows.push_back(arow);
    }
  }

  int rowCount() const { return this->Rows.size(); }

  void setToolTips(const QStringList& txts)
  {
    assert(this->Rows.size() == txts.size());
    for (int cc = 0, max = this->Rows.size(); cc < max; ++cc)
    {
      this->Rows[cc].ToolTipEdit->setText(txts[cc]);
    }
  }

  QStringList toolTips() const
  {
    QStringList tips;
    for (const auto& arow : this->Rows)
    {
      tips.push_back(arow.ToolTipEdit->text());
    }
    return tips;
  }

  void setToolTip(int index, const QString& txt)
  {
    assert(index >= 0 && index < this->Rows.size());
    this->Rows[index].ToolTipEdit->setText(txt);
    this->Rows[index].ToolTipEdit->selectAll();
    this->Rows[index].ToolTipEdit->setFocus();
  }

  QString toolTip(int index) const
  {
    assert(index >= 0 && index < this->Rows.size());
    return this->Rows[index].ToolTipEdit->text();
  }

  void deleteRow(int index)
  {
    assert(index >= 0 && index < this->Rows.size());
    auto& arow = this->Rows[index];
    this->gridLayout->removeWidget(arow.IndexLabel);
    this->gridLayout->removeWidget(arow.ToolTipEdit);
    this->gridLayout->removeWidget(arow.AssignButton);
    if (arow.DeleteButton)
    {
      this->gridLayout->removeWidget(arow.DeleteButton);
    }
    delete arow.IndexLabel;
    delete arow.ToolTipEdit;
    delete arow.AssignButton;
    delete arow.DeleteButton;
    this->Rows.removeAt(index);

    // now update names and widget layout in the grid
    for (int cc = index; cc < this->Rows.size(); ++cc)
    {
      auto& currentRow = this->Rows[cc];
      currentRow.IndexLabel->setText(QString::number(cc + 1));
      currentRow.AssignButton->setProperty("pqCustomViewpointButtonDialog_INDEX", cc);
      this->gridLayout->addWidget(currentRow.IndexLabel, cc + 1, 0);
      this->gridLayout->addWidget(currentRow.ToolTipEdit, cc + 1, 1);
      this->gridLayout->addWidget(currentRow.AssignButton, cc + 1, 2);
      if (currentRow.DeleteButton)
      {
        currentRow.DeleteButton->setProperty("pqCustomViewpointButtonDialog_INDEX", cc);
        this->gridLayout->addWidget(currentRow.DeleteButton, cc + 1, 3);
      }
    }

    // enable/disable add button.
    this->add->setEnabled(
      this->Rows.size() < ::pqCustomViewpointButtonDialog::MAXIMUM_NUMBER_OF_ITEMS);
  }
};

// Organizes button config file info in a single location.
//=============================================================================
class pqCustomViewpointButtonFileInfo
{
public:
  pqCustomViewpointButtonFileInfo()
    : FileIdentifier("CustomViewpointsConfiguration")
    , FileDescription("Custom Viewpoints Configuration")
    , FileExtension(".pvcvbc")
    , WriterVersion("1.0")
  {
  }
  const char* FileIdentifier;
  const char* FileDescription;
  const char* FileExtension;
  const char* WriterVersion;
};

//------------------------------------------------------------------------------
const QString pqCustomViewpointButtonDialog::DEFAULT_TOOLTIP = QString("Unnamed Viewpoint");
const int pqCustomViewpointButtonDialog::MINIMUM_NUMBER_OF_ITEMS = 0;
const int pqCustomViewpointButtonDialog::MAXIMUM_NUMBER_OF_ITEMS = 30;

//------------------------------------------------------------------------------
pqCustomViewpointButtonDialog::pqCustomViewpointButtonDialog(QWidget* Parent, Qt::WindowFlags flags,
  QStringList& toolTips, QStringList& configs, QString& curConfig)
  : QDialog(Parent, flags)
  , ui(nullptr)
{
  this->ui = new pqCustomViewpointButtonDialogUI(this);
  this->ui->setupUi(this);
  this->setToolTipsAndConfigurations(toolTips, configs);
  this->setCurrentConfiguration(curConfig);
  QObject::connect(this->ui->add, SIGNAL(clicked()), this, SLOT(appendRow()));
  QObject::connect(this->ui->clearAll, SIGNAL(clicked()), this, SLOT(clearAll()));
  QObject::connect(this->ui->importAll, SIGNAL(clicked()), this, SLOT(importConfigurations()));
  QObject::connect(this->ui->exportAll, SIGNAL(clicked()), this, SLOT(exportConfigurations()));
}

//------------------------------------------------------------------------------
pqCustomViewpointButtonDialog::~pqCustomViewpointButtonDialog()
{
  delete this->ui;
  this->ui = nullptr;
}

//------------------------------------------------------------------------------
void pqCustomViewpointButtonDialog::setToolTipsAndConfigurations(
  const QStringList& toolTips, const QStringList& configs)
{
  if (toolTips.size() != configs.size())
  {
    qWarning("`setToolTipsAndConfigurations` called with mismatched lengths.");
  }

  int minSize = std::min(toolTips.size(), configs.size());
  if (minSize > MAXIMUM_NUMBER_OF_ITEMS)
  {
    qWarning() << "configs greater than " << MAXIMUM_NUMBER_OF_ITEMS << " will be ignored.";
    minSize = MAXIMUM_NUMBER_OF_ITEMS;
  }

  QStringList realToolTips = toolTips.mid(0, minSize);
  QStringList realConfigs = configs.mid(0, minSize);

  // ensure there are at least MINIMUM_NUMBER_OF_ITEMS items.
  for (int cc = minSize; cc < MINIMUM_NUMBER_OF_ITEMS; ++cc)
  {
    realToolTips.push_back(DEFAULT_TOOLTIP);
    realConfigs.push_back(QString());
  }

  this->ui->setNumberOfRows(realToolTips.size());
  this->ui->setToolTips(realToolTips);
  this->setConfigurations(realConfigs);
}

//------------------------------------------------------------------------------
void pqCustomViewpointButtonDialog::setToolTips(const QStringList& toolTips)
{
  if (toolTips.length() != this->ui->rowCount())
  {
    pqErrorMacro("Error: Wrong number of tool tips.");
    return;
  }
  this->ui->setToolTips(toolTips);
}

//------------------------------------------------------------------------------
QStringList pqCustomViewpointButtonDialog::getToolTips()
{
  return this->ui->toolTips();
}

//------------------------------------------------------------------------------
void pqCustomViewpointButtonDialog::setConfigurations(const QStringList& configs)
{
  if (configs.length() != this->ui->rowCount())
  {
    pqErrorMacro("Error: Wrong number of configurations.");
    return;
  }
  this->Configurations = configs;
}

//------------------------------------------------------------------------------
QStringList pqCustomViewpointButtonDialog::getConfigurations()
{
  return this->Configurations;
}

//------------------------------------------------------------------------------
void pqCustomViewpointButtonDialog::setCurrentConfiguration(const QString& config)
{
  this->CurrentConfiguration = config;
}

//------------------------------------------------------------------------------
QString pqCustomViewpointButtonDialog::getCurrentConfiguration()
{
  return this->CurrentConfiguration;
}

//------------------------------------------------------------------------------
void pqCustomViewpointButtonDialog::importConfigurations()
{
  // What follows is a reader for an xml format that contains
  // a group of nested Camera Configuration XML hierarchies
  // each written by the vtkSMCameraConfigurationWriter.
  // The nested configuration hierarchies might be empty.
  pqCustomViewpointButtonFileInfo fileInfo;

  QString filters =
    QString("%1 (*%2);;All Files (*.*)").arg(fileInfo.FileDescription).arg(fileInfo.FileExtension);

  pqFileDialog dialog(nullptr, this, "Load Custom Viewpoints Configuration", "", filters);
  dialog.setFileMode(pqFileDialog::ExistingFile);

  if (dialog.exec() == QDialog::Accepted)
  {
    QString filename;
    filename = dialog.getSelectedFiles()[0];

    pugi::xml_document doc;
    auto result = doc.load_file(filename.toLocal8Bit().data());
    if (!result)
    {
      qCritical() << "XML Parsing errors (" << filename << ")\n\n"
                  << "Error description: " << result.description() << "\n"
                  << "Error offset: " << result.offset;
      return;
    }

    // check type
    auto root = doc.child(fileInfo.FileIdentifier);
    if (!root)
    {
      pqErrorMacro(<< "This is not a valid " << fileInfo.FileDescription << " XML hierarchy.");
      return;
    }

    // check version
    if (!root.attribute("version"))
    {
      pqErrorMacro("No version attribute was found.");
      return;
    }
    if (strcmp(root.attribute("version").value(), fileInfo.WriterVersion) != 0)
    {
      pqErrorMacro("Unsupported version " << root.attribute("version").value() << ".");
      return;
    }

    // read buttons, their toolTips, and configurations.
    QStringList toolTips;
    QStringList configs;

    for (auto button : root.children())
    {
      if (strncmp(button.name(), "CustomViewpointButton", strlen("CustomViewpointButton")) != 0)
      {
        qWarning() << "Unexpected element found '" << button.name() << "'. Skipping.";
        continue;
      }

      // tool tip
      auto tip = button.child("ToolTip");
      if (!tip)
      {
        pqErrorMacro(<< button.name() << " is missing ToolTip.");
        return;
      }

      auto tipValue = tip.attribute("value");
      if (!tipValue)
      {
        pqErrorMacro("In " << button.name() << " ToolTip is missing value attribute.");
        return;
      }

      toolTips << tipValue.value();

      // Here are the optionally nested Camera Configurations.
      auto config = button.child("Configuration");
      if (!config)
      {
        pqErrorMacro(<< button.name() << " is missing Configuration.");
        return;
      }

      if (config.attribute("is_empty").as_int(1) == 1)
      {
        // Configuration for this button is un-assigned.
        configs << QString();
      }
      else
      {
        vtkSMCameraConfigurationFileInfo pvccInfo;

        // Configuration for this button is assigned.
        auto cameraConfig = config.child(pvccInfo.FileIdentifier);
        if (!cameraConfig)
        {
          pqErrorMacro(<< "In " << button.name() << " invalid " << pvccInfo.FileDescription << ".");
          return;
        }

        // Extract configuration hierarchy from the stream
        // we should validate it but PV state doesn't support this.
        std::ostringstream os;
        cameraConfig.print(os, "  ");
        configs << os.str().c_str();
      }
    }

    // Pass the newly loaded configuration to the GUI.
    this->setToolTipsAndConfigurations(toolTips, configs);
    // this->accept();
  }
}

//------------------------------------------------------------------------------
void pqCustomViewpointButtonDialog::exportConfigurations()
{
  pqCustomViewpointButtonFileInfo fileInfo;

  QString filters =
    QString("%1 (*%2);;All Files (*.*)").arg(fileInfo.FileDescription).arg(fileInfo.FileExtension);

  pqFileDialog dialog(nullptr, this, "Save Custom Viewpoints Configuration", "", filters);
  dialog.setFileMode(pqFileDialog::AnyFile);

  if (dialog.exec() == QDialog::Accepted)
  {
    QString filename;
    filename = dialog.getSelectedFiles()[0];

    pugi::xml_document doc;
    auto root = doc.append_child(fileInfo.FileIdentifier);
    root.append_attribute("version").set_value(fileInfo.WriterVersion);

    const QStringList toolTipTexts = this->ui->toolTips();
    assert(toolTipTexts.size() == this->Configurations.size());
    for (int i = 0, max = this->ui->rowCount(); i < max; ++i)
    {
      auto button =
        root.append_child(QString("CustomViewpointButton%1").arg(i).toStdString().c_str());

      // tool tip
      auto tip = button.append_child("ToolTip");
      tip.append_attribute("value").set_value(toolTipTexts[i].toStdString().c_str());

      // camera configuration
      auto config = button.append_child("Configuration");
      config.append_attribute("is_empty").set_value(this->Configurations[i].isEmpty() ? 1 : 0);

      if (!this->Configurations[i].isEmpty())
      {
        pugi::xml_document camConfigDoc;
        camConfigDoc.load_string(this->Configurations[i].toStdString().c_str());
        config.append_copy(camConfigDoc.first_child());
      }
    }
    if (!doc.save_file(filename.toLocal8Bit().data(), /*indent =*/"  "))
    {
      qCritical() << "Failed to save '" << filename << "'.";
    }
  }
}

//------------------------------------------------------------------------------
void pqCustomViewpointButtonDialog::appendRow()
{
  const int numRows = this->ui->rowCount();
  assert(numRows < MAXIMUM_NUMBER_OF_ITEMS);
  this->ui->setNumberOfRows(numRows + 1);
  this->Configurations.push_back(QString());
}

//------------------------------------------------------------------------------
void pqCustomViewpointButtonDialog::clearAll()
{
  this->setToolTipsAndConfigurations(QStringList(), QStringList());
}

//------------------------------------------------------------------------------
void pqCustomViewpointButtonDialog::assignCurrentViewpoint()
{
  int row = -1;
  if (QObject* asender = this->sender())
  {
    row = asender->property("pqCustomViewpointButtonDialog_INDEX").toInt();
  }

  if (row >= 0 && row < this->ui->rowCount())
  {
    this->Configurations[row] = this->CurrentConfiguration;
    if (this->ui->toolTip(row) == pqCustomViewpointButtonDialog::DEFAULT_TOOLTIP)
    {
      this->ui->setToolTip(row, "Current Viewpoint " + QString::number(row + 1));
    }
  }
}

//------------------------------------------------------------------------------
void pqCustomViewpointButtonDialog::deleteRow()
{
  int row = -1;
  if (QObject* asender = this->sender())
  {
    row = asender->property("pqCustomViewpointButtonDialog_INDEX").toInt();
  }

  if (row >= 0 && row < this->ui->rowCount())
  {
    this->ui->deleteRow(row);
    this->Configurations.removeAt(row);
  }
}
