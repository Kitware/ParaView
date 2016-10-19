#include "pqCustomViewButtonDialog.h"

#include "ui_pqCustomViewButtonDialog.h"

#include <QDebug>

#include "vtkIndent.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMCameraConfigurationFileInfo.h"
#include "vtkSmartPointer.h"

#include "pqFileDialog.h"

#include <sstream>
#include <string>

#define pqErrorMacro(estr)                                                                         \
  qDebug() << "Error in:" << endl << __FILE__ << ", line " << __LINE__ << endl << "" estr << endl;

// User interface
//=============================================================================
class pqCustomViewButtonDialogUI : public Ui::pqCustomViewButtonDialog
{
};

// Organizes button config file info in a single location.
//=============================================================================
class pqCustomViewButtonFileInfo
{
public:
  pqCustomViewButtonFileInfo()
    : FileIdentifier("CustomViewButtonConfiguration")
    , FileDescription("Custom View Button Configuration")
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
const QString pqCustomViewButtonDialog::DEFAULT_TOOLTIP = QString("not configured.");

//------------------------------------------------------------------------------
pqCustomViewButtonDialog::pqCustomViewButtonDialog(QWidget* Parent, Qt::WindowFlags flags,
  QStringList& toolTips, QStringList& configs, QString& curConfig)
  : QDialog(Parent, flags)
  , NButtons(0)
  , ui(0)
{
  this->ui = new pqCustomViewButtonDialogUI;
  this->ui->setupUi(this);

  this->ToolTips << this->ui->toolTip0 << this->ui->toolTip1 << this->ui->toolTip2
                 << this->ui->toolTip3;
  this->NButtons = 4;

  this->setToolTips(toolTips);
  this->setConfigurations(configs);
  this->setCurrentConfiguration(curConfig);

  QObject::connect(this->ui->clearAll, SIGNAL(clicked()), this, SLOT(clearAll()));

  QObject::connect(this->ui->importAll, SIGNAL(clicked()), this, SLOT(importConfigurations()));

  QObject::connect(this->ui->exportAll, SIGNAL(clicked()), this, SLOT(exportConfigurations()));

  QObject::connect(
    this->ui->assignCurrentView0, SIGNAL(clicked()), this, SLOT(assignCurrentView0()));

  QObject::connect(
    this->ui->assignCurrentView1, SIGNAL(clicked()), this, SLOT(assignCurrentView1()));

  QObject::connect(
    this->ui->assignCurrentView2, SIGNAL(clicked()), this, SLOT(assignCurrentView2()));

  QObject::connect(
    this->ui->assignCurrentView3, SIGNAL(clicked()), this, SLOT(assignCurrentView3()));
}

//------------------------------------------------------------------------------
pqCustomViewButtonDialog::~pqCustomViewButtonDialog()
{
  delete this->ui;
  this->ui = NULL;
}

//------------------------------------------------------------------------------
void pqCustomViewButtonDialog::setToolTips(QStringList& toolTips)
{
  if (toolTips.length() != this->NButtons)
  {
    pqErrorMacro("Error: Wrong number of tool tips.");
    return;
  }

  for (int i = 0; i < this->NButtons; ++i)
  {
    this->ToolTips[i]->setText(toolTips[i]);
  }
}

//------------------------------------------------------------------------------
QStringList pqCustomViewButtonDialog::getToolTips()
{
  QStringList toolTips;
  for (int i = 0; i < this->NButtons; ++i)
  {
    toolTips << this->ToolTips[i]->text();
  }
  return toolTips;
}

//------------------------------------------------------------------------------
void pqCustomViewButtonDialog::setConfigurations(QStringList& configs)
{
  if (configs.length() != this->NButtons)
  {
    pqErrorMacro("Error: Wrong number of configurations.");
    return;
  }

  this->Configurations = configs;
}

//------------------------------------------------------------------------------
QStringList pqCustomViewButtonDialog::getConfigurations()
{
  return this->Configurations;
}

//------------------------------------------------------------------------------
void pqCustomViewButtonDialog::setCurrentConfiguration(QString& config)
{
  this->CurrentConfiguration = config;
}

//------------------------------------------------------------------------------
QString pqCustomViewButtonDialog::getCurrentConfiguration()
{
  return this->CurrentConfiguration;
}

//------------------------------------------------------------------------------
void pqCustomViewButtonDialog::importConfigurations()
{
  // What follows is a reader for an xml format that contains
  // a group of nested Camera Configuration XML hierarchies
  // each written by the vtkSMCameraConfigurationWriter.
  // The nested configuration hierarchies might be empty.
  pqCustomViewButtonFileInfo fileInfo;

  QString filters =
    QString("%1 (*%2);;All Files (*.*)").arg(fileInfo.FileDescription).arg(fileInfo.FileExtension);

  pqFileDialog dialog(0, this, "Load Custom View Button Configuration", "", filters);
  dialog.setFileMode(pqFileDialog::ExistingFile);

  if (dialog.exec() == QDialog::Accepted)
  {
    QString filename;
    filename = dialog.getSelectedFiles()[0];

    vtkSmartPointer<vtkPVXMLParser> parser = vtkSmartPointer<vtkPVXMLParser>::New();
    parser->SetFileName(filename.toStdString().c_str());
    if (parser->Parse() == 0)
    {
      pqErrorMacro("Invalid XML in file: " << filename << ".");
      return;
    }

    vtkPVXMLElement* root = parser->GetRootElement();
    if (root == 0)
    {
      pqErrorMacro("Invalid XML in file: " << filename << ".");
      return;
    }

    // check type
    std::string requiredType(fileInfo.FileIdentifier);
    const char* foundType = root->GetName();
    if (foundType == 0 || foundType != requiredType)
    {
      pqErrorMacro(<< "This is not a valid " << fileInfo.FileDescription << " XML hierarchy.");
      return;
    }

    // check version
    const char* foundVersion = root->GetAttribute("version");
    if (foundVersion == 0)
    {
      pqErrorMacro("No version attribute was found.");
      return;
    }
    std::string requiredVersion(fileInfo.WriterVersion);
    if (foundVersion != requiredVersion)
    {
      pqErrorMacro("Unsupported version " << foundVersion << ".");
      return;
    }

    // read buttons, their toolTips, and configurations.
    QStringList toolTips;
    QStringList configs;
    for (int i = 0; i < this->NButtons; ++i)
    {
      // button
      QString buttonTagId = QString("CustomViewButton%1").arg(i);
      vtkPVXMLElement* button = root->FindNestedElementByName(buttonTagId.toStdString().c_str());
      if (button == 0)
      {
        pqErrorMacro("Missing " << buttonTagId << " representation.");
        return;
      }

      // tool tip
      vtkPVXMLElement* tip = button->FindNestedElementByName("ToolTip");
      if (tip == 0)
      {
        pqErrorMacro(<< buttonTagId << " is missing ToolTip.");
        return;
      }
      const char* tipValue = tip->GetAttribute("value");
      if (tipValue == 0)
      {
        pqErrorMacro("In " << buttonTagId << " ToolTip is missing value attribute.");
        return;
      }

      toolTips << tipValue;

      // Here are the optionally nested Camera Configurations.
      vtkPVXMLElement* config = button->FindNestedElementByName("Configuration");
      if (config == 0)
      {
        pqErrorMacro(<< buttonTagId << " is missing Configuration.");
        return;
      }
      const char* isEmptyValue = config->GetAttribute("is_empty");
      if (isEmptyValue == 0)
      {
        pqErrorMacro("In " << buttonTagId << " Configuration is missing is_empty attribute.");
        return;
      }
      int isEmpty;
      std::istringstream is(isEmptyValue);
      is >> isEmpty;
      if (isEmpty)
      {
        // Configuration for this button is un-assigned.
        configs << "";
      }
      else
      {
        vtkSMCameraConfigurationFileInfo pvccInfo;
        // Configuration for this button is assigned.
        vtkPVXMLElement* cameraConfig = config->FindNestedElementByName(pvccInfo.FileIdentifier);
        if (cameraConfig == 0)
        {
          pqErrorMacro(<< "In " << buttonTagId << " invalid " << pvccInfo.FileDescription << ".");
          return;
        }

        // Extract configuration hierarchy from the stream
        // we should validate it but PV state doesn't support this.
        std::ostringstream os;
        cameraConfig->PrintXML(os, vtkIndent());

        configs << os.str().c_str();
      }
    }

    // Pass the newly loaded configuration to the GUI.
    this->setToolTips(toolTips);
    this->setConfigurations(configs);
    // this->accept();
  }
}

//------------------------------------------------------------------------------
void pqCustomViewButtonDialog::exportConfigurations()
{
  pqCustomViewButtonFileInfo fileInfo;

  QString filters =
    QString("%1 (*%2);;All Files (*.*)").arg(fileInfo.FileDescription).arg(fileInfo.FileExtension);

  pqFileDialog dialog(0, this, "Save Custom View Button Configuration", "", filters);
  dialog.setFileMode(pqFileDialog::AnyFile);

  if (dialog.exec() == QDialog::Accepted)
  {
    QString filename;
    filename = dialog.getSelectedFiles()[0];

    vtkPVXMLElement* xmlStream = vtkPVXMLElement::New();
    xmlStream->SetName(fileInfo.FileIdentifier);
    xmlStream->SetAttribute("version", fileInfo.WriterVersion);

    for (int i = 0; i < this->NButtons; ++i)
    {
      // tool tip
      vtkPVXMLElement* tip = vtkPVXMLElement::New();
      tip->SetName("ToolTip");
      tip->SetAttribute("value", this->ToolTips[i]->text().toStdString().c_str());

      // camera configuration
      std::ostringstream os;
      os << (this->Configurations[i].isEmpty() ? 1 : 0);

      vtkPVXMLElement* config = vtkPVXMLElement::New();
      config->SetName("Configuration");
      config->AddAttribute("is_empty", os.str().c_str());

      if (!this->Configurations[i].isEmpty())
      {
        std::string camConfig(this->Configurations[i].toStdString());

        vtkPVXMLParser* parser = vtkPVXMLParser::New();
        parser->InitializeParser();
        parser->ParseChunk(camConfig.c_str(), static_cast<unsigned int>(camConfig.size()));
        parser->CleanupParser();

        vtkPVXMLElement* camConfigXml = parser->GetRootElement();

        config->AddNestedElement(camConfigXml);
        parser->Delete();
      }

      vtkPVXMLElement* button = vtkPVXMLElement::New();
      button->SetName(QString("CustomViewButton%1").arg(i).toStdString().c_str());
      button->AddNestedElement(tip);
      button->AddNestedElement(config);

      xmlStream->AddNestedElement(button);

      tip->Delete();
      config->Delete();
      button->Delete();
    }

    ofstream os(filename.toStdString().c_str(), ios::out);
    xmlStream->PrintXML(os, vtkIndent());
    os << endl;
    os.close();

    xmlStream->Delete();
  }
}

//------------------------------------------------------------------------------
void pqCustomViewButtonDialog::clearAll()
{
  QStringList toolTips;
  toolTips << pqCustomViewButtonDialog::DEFAULT_TOOLTIP << pqCustomViewButtonDialog::DEFAULT_TOOLTIP
           << pqCustomViewButtonDialog::DEFAULT_TOOLTIP
           << pqCustomViewButtonDialog::DEFAULT_TOOLTIP;
  this->setToolTips(toolTips);

  QStringList configs;
  configs << ""
          << ""
          << ""
          << "";
  this->setConfigurations(configs);
}

//------------------------------------------------------------------------------
void pqCustomViewButtonDialog::assignCurrentView(int id)
{
  this->Configurations[id] = this->CurrentConfiguration;
  if (this->ToolTips[id]->text() == pqCustomViewButtonDialog::DEFAULT_TOOLTIP)
  {
    this->ToolTips[id]->setText("Current View " + QString::number(id + 1));
  }
  this->ToolTips[id]->selectAll();
  this->ToolTips[id]->setFocus();
}
