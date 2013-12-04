/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "pqSQHemisphereSource.h"
#include "vtkSQHemisphereSourceConfigurationReader.h"
#include "vtkSQHemisphereSourceConfigurationWriter.h"
#include "pqSQMacros.h"

#include "pqProxy.h"
#include "pqPropertyLinks.h"
#include "pqFileDialog.h"

#include "vtkSMProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkMath.h"


#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"

#include <QString>
#include <QMessageBox>
#include <QFileDialog>
#include <QDoubleValidator>
#include <QLineEdit>
#include <QPalette>
#include <QSettings>
#include <QDebug>

#include "FsUtils.h"
#if defined pqSQHemisphereSourceDEBUG
#include <PrintUtils.h>
#endif

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

//-----------------------------------------------------------------------------
pqSQHemisphereSource::pqSQHemisphereSource(
      pqProxy* l_proxy,
      QWidget* l_parent)
             :
      pqNamedObjectPanel(l_proxy,l_parent)
{
  #if defined pqSQHemisphereSourceDEBUG
  std::cerr << ":::::pqSQHemisphereSource::pqSQHemisphereSource" << std::endl;
  #endif

  // Construct Qt form.
  this->Form=new pqSQHemisphereSourceForm;
  this->Form->setupUi(this);

  // set up coordinate line edits
  this->Form->c_x->setValidator(new QDoubleValidator(this->Form->c_x));
  this->Form->c_y->setValidator(new QDoubleValidator(this->Form->c_y));
  this->Form->c_z->setValidator(new QDoubleValidator(this->Form->c_z));

  this->Form->n_x->setValidator(new QDoubleValidator(this->Form->n_x));
  this->Form->n_y->setValidator(new QDoubleValidator(this->Form->n_y));
  this->Form->n_z->setValidator(new QDoubleValidator(this->Form->n_z));

  this->Form->r->setValidator(new QDoubleValidator(this->Form->r));

  // Set up configuration viewer
  this->PullServerConfig();

  // set up save/restore buttons
  QObject::connect(
      this->Form->save,
      SIGNAL(clicked()),
      this,SLOT(saveConfiguration()));

  QObject::connect(
      this->Form->restore,
      SIGNAL(clicked()),
      this,SLOT(loadConfiguration()));

  // link qt to sm
  this->Links = new pqPropertyLinks;
  this->Links->setUseUncheckedProperties(false);
  this->Links->setAutoUpdateVTKObjects(true);

  QObject::connect(
      this->Links,
      SIGNAL(qtWidgetChanged()),
      this,
      SLOT(setModified()));

  vtkSMProxy* pProxy=this->referenceProxy()->getProxy();

  this->Links->addPropertyLink(
      this->Form->c_x,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Center"),
      0);

  this->Links->addPropertyLink(
      this->Form->c_y,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Center"),
      1);

  this->Links->addPropertyLink(
      this->Form->c_z,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Center"),
      2);

  this->Links->addPropertyLink(
      this->Form->n_x,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("North"),
      0);

  this->Links->addPropertyLink(
      this->Form->n_y,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("North"),
      1);

  this->Links->addPropertyLink(
      this->Form->n_z,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("North"),
      2);

  this->Links->addPropertyLink(
      this->Form->r,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Radius"));

  this->Links->addPropertyLink(
      this->Form->res,
      "value",
      SIGNAL(valueChanged(int)),
      pProxy,
      pProxy->GetProperty("Resolution"));

}

//-----------------------------------------------------------------------------
pqSQHemisphereSource::~pqSQHemisphereSource()
{
  #if defined pqSQHemisphereSourceDEBUG
  std::cerr << ":::::pqSQHemisphereSource::~pqSQHemisphereSource" << std::endl;
  #endif

  delete this->Form;
  delete this->Links;
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::Restore()
{
  QSettings settings("SciberQuest", "SciberQuestToolKit");
  QString lastUsedDir=settings.value("SQHemisphereSource/lastUsedDir","").toString();

  QString fn=QFileDialog::getOpenFileName(this,"Open SQ Hemisphere Source",lastUsedDir,"*.sqhs");
  if (fn.size())
    {
    std::ifstream f(fn.toStdString().c_str(),std::ios_base::in);
    if (f.is_open())
      {
      char buf[1024];
      f.getline(buf,1024);
      if (std::string(buf).find("SQ Hemisphere Source")!=std::string::npos)
        {
        // Center
        f.getline(buf,1024);
        f.getline(buf,1024);
        std::istringstream *is;
        is=new std::istringstream(buf);
        double c[3];
        *is >> c[0] >> c[1] >> c[2];
        delete is;
        this->Form->c_x->setText(QString("%1").arg(c[0]));
        this->Form->c_y->setText(QString("%1").arg(c[1]));
        this->Form->c_z->setText(QString("%1").arg(c[2]));
        // North
        f.getline(buf,1024);
        f.getline(buf,1024);
        is=new std::istringstream(buf);
        double n[3];
        *is >> n[0] >> n[1] >> n[2];
        delete is;
        this->Form->n_x->setText(QString("%1").arg(n[0]));
        this->Form->n_y->setText(QString("%1").arg(n[1]));
        this->Form->n_z->setText(QString("%1").arg(n[2]));
        // Radius
        f.getline(buf,1024);
        f.getline(buf,1024);
        is=new std::istringstream(buf);
        double r;
        *is >> r;
        delete is;
        this->Form->r->setText(QString("%1").arg(r));
        // Resolution
        f.getline(buf,1024);
        f.getline(buf,1024);
        is=new std::istringstream(buf);
        int res;
        *is >> res;
        delete is;
        this->Form->res->setValue(res);
        }
      else
        {
        QMessageBox::warning(
            this,
            "Open SQ Hemisphere Source",
            "Error: Bad format not a SQ plane source file.");
        }
      f.close();
      }
    else
      {
      QMessageBox::warning(
            this,
            "Save SQ Hemisphere Source",
            "Error: Could not open the file.");
      }
    }
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::Save()
{
  QString fn
    = QFileDialog::getSaveFileName(this,"Save SQ Hemisphere Source","","*.sqhs");

  if (fn.size())
    {
    QString lastUsedDir(QFileInfo(fn).path());
    QSettings settings("SciberQuest", "SciberQuestToolKit");
    settings.setValue("SQHemisphereSource/lastUsedDir",lastUsedDir);

    std::ofstream f(fn.toStdString().c_str(),std::ios_base::out|std::ios_base::trunc);
    if (f.is_open())
      {
      f << "SQ Hemisphere Source 1.0" << std::endl
        << "Center" << std::endl
        << this->Form->c_x->text().toDouble() << " "
        << this->Form->c_y->text().toDouble() << " "
        << this->Form->c_z->text().toDouble() << std::endl
        << "North" << std::endl
        << this->Form->n_x->text().toDouble() << " "
        << this->Form->n_y->text().toDouble() << " "
        << this->Form->n_z->text().toDouble() << std::endl
        << "Radius" << std::endl
        << this->Form->r->text().toDouble() << std::endl
        << "Resolution" << std::endl
        << this->Form->res->value() << std::endl
        << std::endl;
      f.close();
      }
    else
      {
      QMessageBox::warning(
            this,
            "Save SQ Hemisphere Source",
            "Error: Failed to create the file.");
      }
    }
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::loadConfiguration()
{
  vtkSQHemisphereSourceConfigurationReader *reader
    = vtkSQHemisphereSourceConfigurationReader::New();

  reader->SetProxy(this->proxy());

  QString filters
    = QString("%1 (*%2);;All Files (*.*)")
        .arg(reader->GetFileDescription()).arg(reader->GetFileExtension());

  pqFileDialog dialog(0,this,"Load SQ Hemisphere Source Configuration","",filters);
  dialog.setFileMode(pqFileDialog::ExistingFile);

  if (dialog.exec()==QDialog::Accepted)
    {
    QString filename;
    filename=dialog.getSelectedFiles()[0];

    int ok=reader->ReadConfiguration(filename.toStdString().c_str());
    if (!ok)
      {
      pqSQErrorMacro(qDebug(),"Failed to load the hemisphere source configuration.");
      }
    }

  reader->Delete();

  this->PullServerConfig();
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::saveConfiguration()
{
  #if defined pqSQHemisphereSourceDEBUG
  std::cerr << ":::::pqSQHemisphereSource::saveConfiguration" << std::endl;
  #endif

  vtkSQHemisphereSourceConfigurationWriter *writer
    = vtkSQHemisphereSourceConfigurationWriter::New();

  writer->SetProxy(this->proxy());

  QString filters
    = QString("%1 (*%2);;All Files (*.*)")
        .arg(writer->GetFileDescription()).arg(writer->GetFileExtension());

  pqFileDialog dialog(0,this,"Save SQ Hemisphere Source Configuration","",filters);
  dialog.setFileMode(pqFileDialog::AnyFile);

  if (dialog.exec()==QDialog::Accepted)
    {
    QString filename(dialog.getSelectedFiles()[0]);

    int ok=writer->WriteConfiguration(filename.toStdString().c_str());
    if (!ok)
      {
      pqSQErrorMacro(qDebug(),"Failed to save the hemisphere source configuration.");
      }
    }

  writer->Delete();
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::PullServerConfig()
{
  #if defined pqSQHemisphereSourceDEBUG
  std::cerr << ":::::pqSQHemisphereSource::PullServerConfig" << std::endl;
  #endif

  vtkSMProxy* pProxy=this->referenceProxy()->getProxy();

  // Center
  vtkSMDoubleVectorProperty *cProp
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("GetCenter"));
  pProxy->UpdatePropertyInformation(cProp);
  double *c=cProp->GetElements();
  this->Form->c_x->setText(QString("%1").arg(c[0]));
  this->Form->c_y->setText(QString("%1").arg(c[1]));
  this->Form->c_z->setText(QString("%1").arg(c[2]));

  // North
  vtkSMDoubleVectorProperty *nProp
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("GetNorth"));
  pProxy->UpdatePropertyInformation(nProp);
  double *n=nProp->GetElements();
  this->Form->n_x->setText(QString("%1").arg(n[0]));
  this->Form->n_y->setText(QString("%1").arg(n[1]));
  this->Form->n_z->setText(QString("%1").arg(n[2]));

  // Radius
  vtkSMDoubleVectorProperty *rProp
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("GetRadius"));
  pProxy->UpdatePropertyInformation(rProp);
  double r=rProp->GetElement(0);
  this->Form->r->setText(QString("%1").arg(r));

  // Resolution
  vtkSMIntVectorProperty *resProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("GetResolution"));
  pProxy->UpdatePropertyInformation(resProp);
  int res=resProp->GetElement(0);
  this->Form->res->setValue(res);

  #if defined pqSQHemisphereSourceDEBUG
  std::cerr << "Pulled: " << std::endl
            << "C   " << c[0] << ", " << c[1] << ", " << c[2] << std::endl
            << "N   " << n[0] << ", " << n[1] << ", " << n[2] << std::endl
            << "r   " << r << std::endl
            << "res " << res << std::endl
            << std::endl;
  #endif
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::PushServerConfig()
{
  #if defined pqSQHemisphereSourceDEBUG
  std::cerr << ":::::pqSQHemisphereSource::PushServerConfig" << std::endl;
  #endif
  vtkSMProxy* pProxy=this->referenceProxy()->getProxy();

  // Center
  double c[3];
  c[0]=this->Form->c_x->text().toDouble();
  c[1]=this->Form->c_y->text().toDouble();
  c[2]=this->Form->c_z->text().toDouble();
  vtkSMDoubleVectorProperty *cProp
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("Center"));
  cProp->SetElements(c,3);

  // North
  double n[3];
  n[0]=this->Form->n_x->text().toDouble();
  n[1]=this->Form->n_y->text().toDouble();
  n[2]=this->Form->n_z->text().toDouble();
  vtkSMDoubleVectorProperty *nProp
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("North"));
  nProp->SetElements(n,3);

  // Radius
  double r;
  r=this->Form->r->text().toDouble();
  vtkSMDoubleVectorProperty *rProp
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("Radius"));
  rProp->SetElement(0,r);

  // Resolution
  int res;
  res=this->Form->res->value();
  vtkSMIntVectorProperty *resProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("Resolution"));
  resProp->SetElement(0,res);

  #if defined pqSQHemisphereSourceDEBUG
  std::cerr << "Pushed: " << std::endl
            << "C   " << c[0] << ", " << c[1] << ", " << c[2] << std::endl
            << "N   " << n[0] << ", " << n[1] << ", " << n[2] << std::endl
            << "r   " << r << std::endl
            << "res " << res << std::endl
            << std::endl;
  #endif

  // Let proxy send updated values.
  pProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::accept()
{
  #if defined pqSQHemisphereSourceDEBUG
  std::cerr << ":::::pqSQHemisphereSource::accept" << std::endl;
  #endif

  pqNamedObjectPanel::accept();
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::reset()
{
  #if defined pqSQHemisphereSourceDEBUG
  std::cerr << ":::::pqSQHemisphereSource::reset" << std::endl;
  #endif

  pqNamedObjectPanel::reset();
}
