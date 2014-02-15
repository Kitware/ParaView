/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#include "pqSQPlaneSource.h"

#include "pqSQTranslateDialog.h"
#include "vtkSQPlaneSourceConfigurationReader.h"
#include "vtkSQPlaneSourceConfigurationWriter.h"
#include "vtkSQPlaneSourceConstants.h"
#include "pqSQMacros.h"

#include "pqApplicationCore.h"
#include "pqProxy.h"
#include "pqPropertyLinks.h"
#include "pqSettings.h"
#include "pqRenderView.h"
#include "pqFileDialog.h"

#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkPVXMLParser.h"
#include "vtkMath.h"

#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QClipboard>
#include <QApplication>
#include <QString>
#include <QMessageBox>
#include <QFileDialog>
#include <QDoubleValidator>
#include <QLineEdit>
#include <QPalette>
#include <QSettings>
#include <QDebug>

#include <string>
#include <iostream>
#include "FsUtils.h"
#include <fstream>
#include <sstream>

// #define pqSQPlaneSourceDEBUG

//-----------------------------------------------------------------------------
pqSQPlaneSource::pqSQPlaneSource(
      pqProxy* l_proxy,
      QWidget* widget)
             :
      pqNamedObjectPanel(l_proxy, widget)
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::pqSQPlaneSource" << std::endl;
  #endif

  this->Dims[0]=this->Dims[1]=1.0;
  this->Dx[0]=this->Dx[1]=1.0;
  this->Nx[0]=this->Nx[1]=1;
  this->N[0]=this->N[1]=this->N[2]=0.0;

  // Construct Qt form.
  this->Form=new pqSQPlaneSourceForm;
  this->Form->setupUi(this);

  // set up coordinate line edits
  this->Form->o_x->setValidator(new QDoubleValidator(this->Form->o_x));
  this->Form->o_y->setValidator(new QDoubleValidator(this->Form->o_y));
  this->Form->o_z->setValidator(new QDoubleValidator(this->Form->o_z));
  this->Form->p1_x->setValidator(new QDoubleValidator(this->Form->p1_x));
  this->Form->p1_y->setValidator(new QDoubleValidator(this->Form->p1_y));
  this->Form->p1_z->setValidator(new QDoubleValidator(this->Form->p1_z));
  this->Form->p2_x->setValidator(new QDoubleValidator(this->Form->p2_x));
  this->Form->p2_y->setValidator(new QDoubleValidator(this->Form->p2_y));
  this->Form->p2_z->setValidator(new QDoubleValidator(this->Form->p2_z));
  this->Form->dx->setValidator(new QDoubleValidator(this->Form->dx));
  this->Form->dy->setValidator(new QDoubleValidator(this->Form->dy));

  // set up save/restore buttons
  QObject::connect(
      this->Form->save,
      SIGNAL(clicked()),
      this,
      SLOT(saveConfiguration()));

  QObject::connect(
      this->Form->restore,
      SIGNAL(clicked()),
      this,
      SLOT(loadConfiguration()));

  //
  QObject::connect(
      this->Form->snap,
      SIGNAL(clicked()),
      this,
      SLOT(SnapViewToNormal()));

  // set up the dimension calculator
  QObject::connect(
      this->Form->o_x,
      SIGNAL(textChanged(QString)),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->o_y,
      SIGNAL(textChanged(QString)),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->o_z,
      SIGNAL(textChanged(QString)),
      this, SLOT(DimensionsModified()));
  //
  QObject::connect(
      this->Form->p1_x,
      SIGNAL(textChanged(QString)),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->p1_y,
      SIGNAL(textChanged(QString)),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->p1_z,
      SIGNAL(textChanged(QString)),
      this, SLOT(DimensionsModified()));
  //
  QObject::connect(
      this->Form->p2_x,
      SIGNAL(textChanged(QString)),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->p2_y,
      SIGNAL(textChanged(QString)),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->p2_z,
      SIGNAL(textChanged(QString)),
      this, SLOT(DimensionsModified()));

  // link resolution to spacing
  QObject::connect(
      this->Form->res_x,
      SIGNAL(valueChanged(int)),
      this, SLOT(ResolutionModified()));
  //
  QObject::connect(
      this->Form->res_y,
      SIGNAL(valueChanged(int)),
      this, SLOT(ResolutionModified()));

  // link spacing to resolution
  QObject::connect(
      this->Form->dx,
      SIGNAL(editingFinished()),
      this, SLOT(SpacingModified()));
  //
  QObject::connect(
      this->Form->dy,
      SIGNAL(editingFinished()),
      this, SLOT(SpacingModified()));

  // make sure if uniform pixel aspect is changed the form is updated
  QObject::connect(
      this->Form->aspectLock,
      SIGNAL(toggled(bool)),
      this, SLOT(SpacingModified()));

  // These configure UI for constraints.
  QObject::connect(
      this->Form->constraint,
      SIGNAL(currentIndexChanged(int)),
      this, SLOT(ApplyConstraint()));

  // make sure constrained values get updated in the UI.
  QObject::connect(
      this->Form->o_x,
      SIGNAL(textChanged(QString)),
      this, SLOT(ApplyConstraint()));

  QObject::connect(
      this->Form->o_y,
      SIGNAL(textChanged(QString)),
      this, SLOT(ApplyConstraint()));

  QObject::connect(
      this->Form->o_z,
      SIGNAL(textChanged(QString)),
      this, SLOT(ApplyConstraint()));

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
      this->Form->name,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Name"));

  this->Links->addPropertyLink(
      this->Form->o_x,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Origin"),
      0);

  this->Links->addPropertyLink(
      this->Form->o_y,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Origin"),
      1);

  this->Links->addPropertyLink(
      this->Form->o_z,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Origin"),
      2);

  this->Links->addPropertyLink(
      this->Form->p1_x,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Point1"),
      0);

  this->Links->addPropertyLink(
      this->Form->p1_y,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Point1"),
      1);

  this->Links->addPropertyLink(
      this->Form->p1_z,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Point1"),
      2);

  this->Links->addPropertyLink(
      this->Form->p2_x,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Point2"),
      0);

  this->Links->addPropertyLink(
      this->Form->p2_y,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Point2"),
      1);

  this->Links->addPropertyLink(
      this->Form->p2_z,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Point2"),
      2);

  this->Links->addPropertyLink(
      this->Form->res_x,
      "value",
      SIGNAL(valueChanged(int)),
      pProxy,
      pProxy->GetProperty("XResolution"));

  this->Links->addPropertyLink(
      this->Form->res_y,
      "value",
      SIGNAL(valueChanged(int)),
      pProxy,
      pProxy->GetProperty("YResolution"));

  this->Links->addPropertyLink(
      this->Form->immediateMode,
      "checked",
      SIGNAL(stateChanged(int)),
      pProxy,
      pProxy->GetProperty("ImmediateMode"));

  this->Links->addPropertyLink(
      this->Form->constraint,
      "currentIndex",
      SIGNAL(currentIndexChanged(int)),
      pProxy,
      pProxy->GetProperty("Constraint"));

  this->Links->addPropertyLink(
      this->Form->decompType,
      "currentIndex",
      SIGNAL(currentIndexChanged(int)),
      pProxy,
      pProxy->GetProperty("DecompType"));

}

//-----------------------------------------------------------------------------
pqSQPlaneSource::~pqSQPlaneSource()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::~pqSQPlaneSource" << std::endl;
  #endif

  delete this->Form;
  delete this->Links;
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::contextMenuEvent(QContextMenuEvent *aEvent)
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::contextMenuEvent" << std::endl;
  #endif

  QMenu context(this);

  QAction *copyAct=new QAction(tr("Copy Configuration"),&context);
  connect(copyAct, SIGNAL(triggered()), this, SLOT(CopyConfiguration()));
  context.addAction(copyAct);

  QAction *pasteAct=new QAction(tr("Paste Configuration"),&context);
  connect(pasteAct, SIGNAL(triggered()), this, SLOT(PasteConfiguration()));
  context.addAction(pasteAct);

  QAction *transAct=new QAction(tr("Translate"),&context);
  connect(transAct, SIGNAL(triggered()), this, SLOT(ShowTranslateDialog()));
  context.addAction(transAct);

  context.exec(aEvent->globalPos());
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::CopyConfiguration()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::CopyConfiguration" << std::endl;
  #endif

  // grab the current configuration.
  std::ostringstream os;

  vtkSQPlaneSourceConfigurationWriter *writer
    = vtkSQPlaneSourceConfigurationWriter::New();

  writer->SetProxy(this->proxy());
  writer->WriteConfiguration(os);

  // place it on the clipboard.
  QClipboard *clipboard=QApplication::clipboard();
  clipboard->setText(os.str().c_str());

  writer->Delete();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::PasteConfiguration()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::PasteConfiguration" << std::endl;
  #endif

  QClipboard *clipboard=QApplication::clipboard();
  QString config=clipboard->text();

  if (!config.isEmpty())
    {
    vtkSmartPointer<vtkPVXMLParser> parser=vtkSmartPointer<vtkPVXMLParser>::New();
    parser->InitializeParser();
    parser->ParseChunk(config.toLatin1().data(),static_cast<unsigned int>(config.size()));
    parser->CleanupParser();

    vtkPVXMLElement *xmlStream=parser->GetRootElement();
    if (!xmlStream)
      {
      pqSQErrorMacro(qDebug(),"Invalid SQPlaneSource configuration  pasted.");
      return;
      }

    vtkSmartPointer<vtkSQPlaneSourceConfigurationReader> reader
      = vtkSmartPointer<vtkSQPlaneSourceConfigurationReader>::New();

    reader->SetProxy(this->proxy());
    int ok=reader->ReadConfiguration(xmlStream);
    if (!ok)
      {
      pqSQErrorMacro(qDebug(),"Invalid SQPlaneSource configuration  hierarchy.");
      return;
      }

    this->PullServerConfig();
    }
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::ShowTranslateDialog()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::pqSQPlaneSource::ShowTranslateDialog" << std::endl;
  #endif

  pqSQTranslateDialog dialog(this,0);

  if (dialog.exec()==QDialog::Accepted)
    {
    double t[3]={0.0};
    dialog.GetTranslation(t);

    double o[3]={0.0};
    this->GetOrigin(o);

    if (dialog.GetTypeIsNewOrigin())
      {
      for (int q=0; q<3; ++q)
        {
        t[q]=t[q]-o[q];
        }
      }

    for (int q=0; q<3; ++q)
      {
      o[q]+=t[q];
      }
    this->SetOrigin(o);

    double p1[3]={0.0};
    this->GetPoint1(p1);
    for (int q=0; q<3; ++q)
      {
      p1[q]+=t[q];
      }
    this->SetPoint1(p1);

    double p2[3]={0.0};
    this->GetPoint2(p2);
    for (int q=0; q<3; ++q)
      {
      p2[q]+=t[q];
      }
    this->SetPoint2(p2);
    }
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::Restore()
{
  QSettings settings("SciberQuest", "SciberQuestToolKit");
  QString lastUsedDir=settings.value("SQPlaneSource/lastUsedDir","").toString();

  QString fn=QFileDialog::getOpenFileName(this,"Open SQ Plane Source",lastUsedDir,"*.sqps");
  if (fn.size())
    {
    std::ifstream f(fn.toStdString().c_str(),std::ios_base::in);
    if (f.is_open())
      {
      char buf[1024];
      f.getline(buf,1024);
      if (std::string(buf).find("SQ Plane Source")!=std::string::npos)
        {
        // DescriptiveName
        f.getline(buf,1024);
        f.getline(buf,1024);
        this->Form->name->setText(buf);
        // Origin
        f.getline(buf,1024);
        f.getline(buf,1024);
        std::istringstream *is;
        is=new std::istringstream(buf);
        double p[3];
        *is >> p[0] >> p[1] >> p[2];
        delete is;
        this->Form->o_x->setText(QString("%1").arg(p[0]));
        this->Form->o_y->setText(QString("%1").arg(p[1]));
        this->Form->o_z->setText(QString("%1").arg(p[2]));
        // Point1
        f.getline(buf,1024);
        f.getline(buf,1024);
        is=new std::istringstream(buf);
        *is >> p[0] >> p[1] >> p[2];
        delete is;
        this->Form->p1_x->setText(QString("%1").arg(p[0]));
        this->Form->p1_y->setText(QString("%1").arg(p[1]));
        this->Form->p1_z->setText(QString("%1").arg(p[2]));
        // Point2
        f.getline(buf,1024);
        f.getline(buf,1024);
        is=new std::istringstream(buf);
        *is >> p[0] >> p[1] >> p[2];
        delete is;
        this->Form->p2_x->setText(QString("%1").arg(p[0]));
        this->Form->p2_y->setText(QString("%1").arg(p[1]));
        this->Form->p2_z->setText(QString("%1").arg(p[2]));
        // Resolution
        f.getline(buf,1024);
        f.getline(buf,1024);
        is=new std::istringstream(buf);
        int r[2];
        *is >> r[0] >> r[1];
        delete is;
        this->Form->res_x->setValue(r[0]);
        this->Form->res_y->setValue(r[0]);

        // update derived values.
        this->DimensionsModified();
        }
      else
        {
        QMessageBox::warning(
              this,
              "Open SQ Plane Source",
              "Error: Bad format not a SQ plane source file.");
        }
      f.close();
      }
    else
      {
      QMessageBox::warning(
          this,
          "Save SQ Plane Source",
          "Error: Could not open the file.");
      }
    }
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::Save()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::Save" << std::endl;
  #endif

  QString fn=QFileDialog::getSaveFileName(this,"Save SQ Plane Source","","*.sqps");
  if (fn.size())
    {
    QString lastUsedDir(QFileInfo(fn).path());
    QSettings settings("SciberQuest", "SciberQuestToolKit");
    settings.setValue("SQPlaneSource/lastUsedDir",lastUsedDir);

    std::ofstream f(fn.toStdString().c_str(),std::ios_base::out|std::ios_base::trunc);
    if (f.is_open())
      {
      f << "SQ Plane Source 1.0" << std::endl
        << "DescriptiveName" << std::endl
        << this->Form->name->text().toStdString() << std::endl
        << "Origin" << std::endl
        << this->Form->o_x->text().toDouble() << " "
        << this->Form->o_y->text().toDouble() << " "
        << this->Form->o_z->text().toDouble() << std::endl
        << "Point1" << std::endl
        << this->Form->p1_x->text().toDouble() << " "
        << this->Form->p1_y->text().toDouble() << " "
        << this->Form->p1_z->text().toDouble() << std::endl
        << "Point2" << std::endl
        << this->Form->p2_x->text().toDouble() << " "
        << this->Form->p2_y->text().toDouble() << " "
        << this->Form->p2_z->text().toDouble() << std::endl
        << "Resolution" << std::endl
        << this->Form->res_x->value() << " "
        << this->Form->res_y->value() << std::endl
        << std::endl;
      f.close();
      }
    else
      {
      QMessageBox::warning(this,"Save SQ Plane Source","Error: Failed to create the file.");
      }
    }
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::loadConfiguration()
{
  vtkSQPlaneSourceConfigurationReader *reader=vtkSQPlaneSourceConfigurationReader::New();
  reader->SetProxy(this->proxy());

  QString filters
    = QString("%1 (*%2);;All Files (*.*)")
        .arg(reader->GetFileDescription()).arg(reader->GetFileExtension());

  pqFileDialog dialog(0,this,"Load SQ Plane Source Configuration","",filters);
  dialog.setFileMode(pqFileDialog::ExistingFile);

  if (dialog.exec()==QDialog::Accepted)
    {
    QString filename;
    filename=dialog.getSelectedFiles()[0];

    int ok=reader->ReadConfiguration(filename.toStdString().c_str());
    if (!ok)
      {
      pqSQErrorMacro(qDebug(),"Failed to load the plane source configuration.");
      }
    }

  reader->Delete();

  this->PullServerConfig();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::saveConfiguration()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::saveConfiguration" << std::endl;
  #endif

  vtkSQPlaneSourceConfigurationWriter *writer=vtkSQPlaneSourceConfigurationWriter::New();
  writer->SetProxy(this->proxy());

  QString filters
    = QString("%1 (*%2);;All Files (*.*)")
        .arg(writer->GetFileDescription()).arg(writer->GetFileExtension());

  pqFileDialog dialog(0,this,"Save SQ Plane Source Configuration","",filters);
  dialog.setFileMode(pqFileDialog::AnyFile);

  if (dialog.exec()==QDialog::Accepted)
    {
    QString filename(dialog.getSelectedFiles()[0]);

    int ok=writer->WriteConfiguration(filename.toStdString().c_str());
    if (!ok)
      {
      pqSQErrorMacro(qDebug(),"Failed to save the plane source configuration.");
      }
    }

  writer->Delete();
}

//-----------------------------------------------------------------------------
int pqSQPlaneSource::ValidateCoordinates()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::ValidateCoordinates" << std::endl;
  #endif

  double n[3]={0.0};
  int ok=this->CalculateNormal(n);
  if (ok)
    {
    this->Form->coordStatus->setText("OK");
    this->Form->coordStatus->setStyleSheet("color:green; background-color:white;");
    }
  else
    {
    this->Form->coordStatus->setText("Error");
    this->Form->coordStatus->setStyleSheet("color:red; background-color:lightyellow;");
    this->Form->n_x->setText("");
    this->Form->n_y->setText("");
    this->Form->n_z->setText("");
    this->Form->dim_x->setText("");
    this->Form->dim_y->setText("");
    }
  return ok;
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::GetOrigin(double *o)
{
  o[0]=this->Form->o_x->text().toDouble();
  o[1]=this->Form->o_y->text().toDouble();
  o[2]=this->Form->o_z->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SetOrigin(double *o)
{
  this->Form->o_x->setText(QString("%1").arg(o[0]));
  this->Form->o_y->setText(QString("%1").arg(o[1]));
  this->Form->o_z->setText(QString("%1").arg(o[2]));
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::GetPoint1(double *p1)
{
  p1[0]=this->Form->p1_x->text().toDouble();
  p1[1]=this->Form->p1_y->text().toDouble();
  p1[2]=this->Form->p1_z->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SetPoint1(double *p1)
{
  this->Form->p1_x->setText(QString("%1").arg(p1[0]));
  this->Form->p1_y->setText(QString("%1").arg(p1[1]));
  this->Form->p1_z->setText(QString("%1").arg(p1[2]));
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::GetPoint2(double *p2)
{
  p2[0]=this->Form->p2_x->text().toDouble();
  p2[1]=this->Form->p2_y->text().toDouble();
  p2[2]=this->Form->p2_z->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SetPoint2(double *p2)
{
  this->Form->p2_x->setText(QString("%1").arg(p2[0]));
  this->Form->p2_y->setText(QString("%1").arg(p2[1]));
  this->Form->p2_z->setText(QString("%1").arg(p2[2]));
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::GetResolution(int *res)
{
  res[0]=this->Form->res_x->text().toInt();
  res[1]=this->Form->res_y->text().toInt();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SetResolution(int *res)
{
  this->Form->res_x->setValue(res[0]);
  this->Form->res_y->setValue(res[1]);
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::GetSpacing(double *dx)
{
  dx[0]=this->Form->dx->text().toDouble();
  dx[1]=this->Form->dy->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SetSpacing(double *dx)
{
  this->Form->dx->setText(QString("%1").arg(dx[0]));
  this->Form->dy->setText(QString("%1").arg(dx[1]));
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::GetNormal(double *n)
{
  n[0]=this->Form->p2_x->text().toDouble();
  n[1]=this->Form->p2_y->text().toDouble();
  n[2]=this->Form->p2_z->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SetNormal(double *n)
{
  this->Form->n_x->setText(QString("%1").arg(n[0]));
  this->Form->n_y->setText(QString("%1").arg(n[1]));
  this->Form->n_z->setText(QString("%1").arg(n[2]));
}

//-----------------------------------------------------------------------------
int pqSQPlaneSource::CalculateNormal(double *n)
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::CaclulateNormal" << std::endl;
  #endif

  this->Form->coordStatus->setText("OK");
  this->Form->coordStatus->setStyleSheet("color:green; background-color:white;");

  double o[3];
  this->GetOrigin(o);

  double p1[3];
  this->GetPoint1(p1);

  double p2[3];
  this->GetPoint2(p2);

  double v1[3];
  v1[0]=p1[0]-o[0];
  v1[1]=p1[1]-o[1];
  v1[2]=p1[2]-o[2];

  double v2[3];
  v2[0]=p2[0]-o[0];
  v2[1]=p2[1]-o[1];
  v2[2]=p2[2]-o[2];

  int ok=1;
  vtkMath::Cross(v1,v2,n);
  double norm=vtkMath::Normalize(n);
  if (norm<=1.0e-6)
    {
    ok=0;
    this->Form->coordStatus->setText("Error");
    this->Form->coordStatus->setStyleSheet("color:red; background-color:lightyellow;");
    this->Form->n_x->setText("Error");
    this->Form->n_y->setText("Error");
    this->Form->n_z->setText("Error");
    this->Form->nCells->setText("Error");
    this->Form->dim_x->setText("Error");
    this->Form->dim_y->setText("Error");
    }

  return ok;
}

//-----------------------------------------------------------------------------
int pqSQPlaneSource::GetDecompType()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::GetDecompType" << std::endl;
  #endif

  return this->Form->constraint->currentIndex();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SetDecompType(int type)
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::SetDecompType" << std::endl;
  #endif

  this->Form->constraint->setCurrentIndex(type);
}

//-----------------------------------------------------------------------------
int pqSQPlaneSource::GetConstraint()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::GetConstraint" << std::endl;
  #endif

  return this->Form->constraint->currentIndex();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SetConstraint(int type)
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::SetConstraint" << std::endl;
  #endif

  this->Form->constraint->setCurrentIndex(type);
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::ApplyConstraint()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::ApplyConstraint" << std::endl;
  #endif

  int type=this->Form->constraint->currentIndex();
  switch(type)
    {
    case SQPS_CONSTRAINT_NONE:
      this->Form->p1_x->setEnabled(true);
      this->Form->p1_y->setEnabled(true);
      this->Form->p1_z->setEnabled(true);

      this->Form->p2_x->setEnabled(true);
      this->Form->p2_y->setEnabled(true);
      this->Form->p2_z->setEnabled(true);
      break;

    case SQPS_CONSTRAINT_XY:
      this->Form->p1_x->setEnabled(true);
      this->Form->p1_y->setEnabled(true);
      this->Form->p1_z->setEnabled(false);
      this->Form->p1_z->setText(this->Form->o_z->text());

      this->Form->p2_x->setEnabled(true);
      this->Form->p2_y->setEnabled(true);
      this->Form->p2_z->setEnabled(false);
      this->Form->p2_z->setText(this->Form->o_z->text());
      break;

    case SQPS_CONSTRAINT_XZ:
      this->Form->p1_x->setEnabled(true);
      this->Form->p1_y->setEnabled(false);
      this->Form->p1_z->setEnabled(true);
      this->Form->p1_y->setText(this->Form->o_y->text());

      this->Form->p2_x->setEnabled(true);
      this->Form->p2_y->setEnabled(false);
      this->Form->p2_z->setEnabled(true);
      this->Form->p2_y->setText(this->Form->o_y->text());
      break;

    case SQPS_CONSTRAINT_YZ:
      this->Form->p1_x->setEnabled(false);
      this->Form->p1_y->setEnabled(true);
      this->Form->p1_z->setEnabled(true);
      this->Form->p1_x->setText(this->Form->o_x->text());

      this->Form->p2_x->setEnabled(false);
      this->Form->p2_y->setEnabled(true);
      this->Form->p2_z->setEnabled(true);
      this->Form->p2_x->setText(this->Form->o_x->text());
      break;

    default:
      pqSQErrorMacro(qDebug(),"Invalid constraint " << type << ".");
      break;
    }
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::DimensionsModified()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::DimensionsModified" << std::endl;
  #endif

  int ok=this->CalculateNormal(this->N);
  if (!ok)
    {
    this->N[0]=this->N[1]=this->N[2]=0.0;
    return;
    }
  this->SetNormal(this->N);


  double o[3];
  this->GetOrigin(o);

  double p1[3];
  this->GetPoint1(p1);

  double p2[3];
  this->GetPoint2(p2);

  double l_x,l_y,l_z,dim;
  l_x=p1[0]-o[0];
  l_y=p1[1]-o[1];
  l_z=p1[2]-o[2];
  dim=sqrt(l_x*l_x+l_y*l_y+l_z*l_z);
  this->Dims[0]=dim;

  l_x=p2[0]-o[0];
  l_y=p2[1]-o[1];
  l_z=p2[2]-o[2];
  dim=sqrt(l_x*l_x+l_y*l_y+l_z*l_z);
  this->Dims[1]=dim;

  this->Form->dim_x->setText(QString("%1").arg(this->Dims[0]));
  this->Form->dim_y->setText(QString("%1").arg(this->Dims[1]));

  // recompute resolution based on the current grid spacing
  // settings.
  //this->SpacingModified();
  this->ResolutionModified();

  return;
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SpacingModified()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::SpacingModified" << std::endl;
  #endif

  // retreive the requested spacing.
  this->GetSpacing(this->Dx);

  // enforce uniform pixel aspect ratio
  if (this->Form->aspectLock->isChecked())
    {
    this->Dx[1]=this->Dx[0];
    this->SetSpacing(this->Dx);
    }

  // update the grid resolution to match the requested grid spacing
  // as closely as possible.
  this->Nx[0]=(int)ceil(this->Dims[0]/this->Dx[0]);
  this->Nx[1]=(int)ceil(this->Dims[1]/this->Dx[1]);

  this->SetResolution(this->Nx);

  // compute the new number of cells.
  int nCells=this->Nx[0]*this->Nx[1];
  this->Form->nCells->setText(QString("%1").arg(nCells));

  this->Links->accept();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::ResolutionModified()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::ResolutionModified" << std::endl;
  #endif

  // retreive the requested resolution.
  this->GetResolution(this->Nx);

  // enforce a uniform pixel aspect ratio
  if (this->Form->aspectLock->isChecked())
    {
    this->Nx[1]=(int)(this->Dims[0]>1E-6?this->Nx[0]*this->Dims[1]/this->Dims[0]:1.0);
    this->Nx[1]=(this->Nx[1]<1?1:this->Nx[1]);
    this->SetResolution(this->Nx);
    }

  // compute the new spacing
  this->Dx[0]=this->Dims[0]/this->Nx[0];
  this->Dx[1]=this->Dims[1]/this->Nx[1];

  this->SetSpacing(this->Dx);

  // compute the new number of cells.
  int nCells=this->Nx[0]*this->Nx[1];
  this->Form->nCells->setText(QString("%1").arg(nCells));

  this->Links->accept();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SnapViewToNormal()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::SnapViewToNormal" << std::endl;
  #endif

  double o[3];
  this->GetOrigin(o);

  double p1[3];
  this->GetPoint1(p1);

  double p2[3];
  this->GetPoint2(p2);

  // compute the plane's center, this will become the camera focal point.
  double a[3];
  a[0]=o[0]+0.5*(p1[0]-o[0]);
  a[1]=o[1]+0.5*(p1[1]-o[1]);
  a[2]=o[2]+0.5*(p1[2]-o[2]);

  double b[3];
  b[0]=o[0]+0.5*(p2[0]-o[0]);
  b[1]=o[1]+0.5*(p2[1]-o[1]);
  b[2]=o[2]+0.5*(p2[2]-o[2]);

  double cen[3];
  cen[0]=a[0]+b[0]-o[0];
  cen[1]=a[1]+b[1]-o[1];
  cen[2]=a[2]+b[2]-o[2];

  // compute the camera center, 2 polane diagonals along its normal from
  // its center.
  double diag
    = sqrt(this->Dims[0]*this->Dims[0]+this->Dims[1]*this->Dims[1]);

  double l_pos[3];
  l_pos[0]=cen[0]+this->N[0]*2.0*diag;
  l_pos[1]=cen[1]+this->N[1]*2.0*diag;
  l_pos[2]=cen[2]+this->N[2]*2.0*diag;

  // compute the camera up from one of the planes axis.
  double up[3];
  if (this->Form->viewUp1->isChecked())
    {
    up[0]=p1[0]-o[0];
    up[1]=p1[1]-o[1];
    up[2]=p1[2]-o[2];
    }
  else
    {
    up[0]=p2[0]-o[0];
    up[1]=p2[1]-o[1];
    up[2]=p2[2]-o[2];
    }
  double mup=sqrt(up[0]*up[0]+up[1]*up[1]+up[2]*up[2]);
  up[0]/=mup;
  up[1]/=mup;
  up[2]/=mup;


  pqRenderView *l_view=dynamic_cast<pqRenderView*>(this->view());
  if (!l_view)
    {
    pqSQErrorMacro(qDebug(),"Failed to get the current view.");
    return;
    }

  vtkSMRenderViewProxy *l_proxy=l_view->getRenderViewProxy();

  vtkSMDoubleVectorProperty *prop;

  prop=dynamic_cast<vtkSMDoubleVectorProperty*>(l_proxy->GetProperty("CameraPosition"));
  prop->SetElements(l_pos);

  prop=dynamic_cast<vtkSMDoubleVectorProperty*>(l_proxy->GetProperty("CameraFocalPoint"));
  prop->SetElements(cen);

  prop=dynamic_cast<vtkSMDoubleVectorProperty*>(l_proxy->GetProperty("CameraViewUp"));
  prop->SetElements(up);

  prop=dynamic_cast<vtkSMDoubleVectorProperty*>(l_proxy->GetProperty("CenterOfRotation"));
  prop->SetElements(cen);

  l_proxy->UpdateVTKObjects();

  l_view->render();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::PullServerConfig()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::PullServerConfig" << std::endl;
  #endif

  vtkSMProxy* pProxy=this->referenceProxy()->getProxy();

  // Name
  vtkSMStringVectorProperty *nameProp
    = dynamic_cast<vtkSMStringVectorProperty*>(pProxy->GetProperty("Name"));
  pProxy->UpdatePropertyInformation(nameProp);
  std::string name=nameProp->GetElement(0);
  if (!name.empty())
    {
    this->Form->name->setText(name.c_str());
    }

  // Origin
  vtkSMDoubleVectorProperty *oProp
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("Origin"));
  pProxy->UpdatePropertyInformation(oProp);
  double *o=oProp->GetElements();
  this->SetOrigin(o);

  // Point 1
  vtkSMDoubleVectorProperty *p1Prop
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("Point1"));
  pProxy->UpdatePropertyInformation(p1Prop);
  double *p1=p1Prop->GetElements();
  this->SetPoint1(p1);

  // Point 2
  vtkSMDoubleVectorProperty *p2Prop
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("Point2"));
  pProxy->UpdatePropertyInformation(p2Prop);
  double *p2=p2Prop->GetElements();
  this->SetPoint2(p2);

  // Resolution
  vtkSMIntVectorProperty *rxProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("XResolution"));
  pProxy->UpdatePropertyInformation(rxProp);
  vtkSMIntVectorProperty *ryProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("YResolution"));
  pProxy->UpdatePropertyInformation(ryProp);
  int res[2]={rxProp->GetElement(0),ryProp->GetElement(0)};
  this->SetResolution(res);

  // Mode
  vtkSMIntVectorProperty *modeProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("ImmediateMode"));
  pProxy->UpdatePropertyInformation(modeProp);
  this->Form->immediateMode->setChecked(modeProp->GetElement(0));

  // Constraints
  vtkSMIntVectorProperty *constraintProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("Constraint"));
  pProxy->UpdatePropertyInformation(constraintProp);
  this->SetConstraint(constraintProp->GetElement(0));

  // DecompTypes
  vtkSMIntVectorProperty *decompProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("DecompType"));
  pProxy->UpdatePropertyInformation(decompProp);
  this->SetDecompType(decompProp->GetElement(0));

  // update derived/computed values.
  this->DimensionsModified();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::PushServerConfig()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::PushServerConfig" << std::endl;
  #endif
  vtkSMProxy* pProxy=this->referenceProxy()->getProxy();

  // Name
  vtkSMStringVectorProperty *nameProp
    = dynamic_cast<vtkSMStringVectorProperty*>(pProxy->GetProperty("Name"));
  nameProp->SetElement(0,this->Form->name->text().toStdString().c_str());

  // Origin
  double o[3];
  this->GetOrigin(o);
  vtkSMDoubleVectorProperty *oProp
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("Origin"));
  oProp->SetElements(o,3);

  // Point 1
  double p1[3];
  this->GetPoint1(p1);
  vtkSMDoubleVectorProperty *p1Prop
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("Point1"));
  p1Prop->SetElements(p1,3);

  // Point 2
  double p2[3];
  this->GetPoint2(p2);
  vtkSMDoubleVectorProperty *p2Prop
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("Point2"));
  p2Prop->SetElements(p2,3);

  // Resolution
  int nx[2];
  this->GetResolution(nx);
  vtkSMIntVectorProperty *rxProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("XResolution"));
  rxProp->SetElement(0,nx[0]);
  vtkSMIntVectorProperty *ryProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("YResolution"));
  ryProp->SetElement(0,nx[1]);

  // Mode
  vtkSMIntVectorProperty *modeProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("ImmediateMode"));
  pProxy->UpdatePropertyInformation(modeProp);
  modeProp->SetElement(0,this->Form->immediateMode->isChecked()?1:0);

  // Constraint
  vtkSMIntVectorProperty *constraintProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("Constraint"));
  pProxy->UpdatePropertyInformation(constraintProp);
  constraintProp->SetElement(0,this->GetConstraint());

  // DecompType
  vtkSMIntVectorProperty *decompProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("DecompType"));
  pProxy->UpdatePropertyInformation(decompProp);
  decompProp->SetElement(0,this->GetDecompType());

  // Let proxy send updated values.
  pProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::accept()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::accept" << std::endl;
  #endif

  if (!this->ValidateCoordinates())
    {
    pqSQErrorMacro(qDebug(),"Invalid coordinate system.");
    }

  pqNamedObjectPanel::accept();
}



//-----------------------------------------------------------------------------
void pqSQPlaneSource::reset()
{
  #if defined pqSQPlaneSourceDEBUG
  std::cerr << ":::::pqSQPlaneSource::reset" << std::endl;
  #endif

  pqNamedObjectPanel::reset();
}
