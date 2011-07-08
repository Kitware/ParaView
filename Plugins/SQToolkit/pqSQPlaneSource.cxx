/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#include "pqSQPlaneSource.h"

#include "vtkSQPlaneSourceConfigurationReader.h"
#include "vtkSQPlaneSourceConfigurationWriter.h"
#include "vtkSQPlaneSourceConstants.h"
#include "SQMacros.h"

#include "pqApplicationCore.h"
#include "pqProxy.h"
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

// #include "vtkEventQtSlotConnect.h"

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
using vtkstd::string;

#include <iostream>
using std::cerr;
using std::endl;

// I think these can be removed when the old configuration
// writing code is cleaned up.
#include "FsUtils.h"
#include <fstream>
using std::ofstream;
using std::ifstream;
using std::ios_base;
#include <sstream>
using std::ostringstream;
using std::istringstream;

//-----------------------------------------------------------------------------
pqSQPlaneSource::pqSQPlaneSource(
      pqProxy* l_proxy,
      QWidget* widget)
             :
      pqNamedObjectPanel(l_proxy, widget)
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQPlaneSource" << endl;
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

  this->SetSpacing(this->Dx);
  this->SetResolution(this->Nx);
  this->SetNormal(this->N);

  this->Form->constraintNone->click();

  //   vtkSMProxy* pProxy=this->referenceProxy()->getProxy();
  //   
  //   // Connect to server side pipeline's UpdateInformation events.
  //   this->VTKConnect=vtkEventQtSlotConnect::New();
  //   this->VTKConnect->Connect(
  //       pProxy,
  //       vtkCommand::UpdateInformationEvent,
  //       this, SLOT(PullServerConfig()));

  // Set up configuration viewer
  this->PullServerConfig();

  // set up save/restore buttons
  QObject::connect(this->Form->save,SIGNAL(clicked()),this,SLOT(saveConfiguration()));
  QObject::connect(this->Form->restore,SIGNAL(clicked()),this,SLOT(loadConfiguration()));
  //QObject::connect(this->Form->restore,SIGNAL(clicked()),this,SLOT(Restore()));
  QObject::connect(this->Form->snap,SIGNAL(clicked()),this,SLOT(SnapViewToNormal()));

  // set up the dimension calculator
  QObject::connect(
      this->Form->o_x,
      SIGNAL(editingFinished()),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->o_y,
      SIGNAL(editingFinished()),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->o_z,
      SIGNAL(editingFinished()),
      this, SLOT(DimensionsModified()));
  //
  QObject::connect(
      this->Form->p1_x,
      SIGNAL(editingFinished()),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->p1_y,
      SIGNAL(editingFinished()),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->p1_z,
      SIGNAL(editingFinished()),
      this, SLOT(DimensionsModified()));
  //
  QObject::connect(
      this->Form->p2_x,
      SIGNAL(editingFinished()),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->p2_y,
      SIGNAL(editingFinished()),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->p2_z,
      SIGNAL(editingFinished()),
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

  // These connection let PV know that we have changed, and makes the apply 
  // button activated.
  QObject::connect(
      this->Form->name,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  //
  QObject::connect(
      this->Form->o_x,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  QObject::connect(
      this->Form->o_y,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  QObject::connect(
      this->Form->o_z,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  //
  QObject::connect(
      this->Form->p1_x,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  QObject::connect(
      this->Form->p1_y,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  QObject::connect(
      this->Form->p1_z,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  //
  QObject::connect(
      this->Form->p2_x,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  QObject::connect(
      this->Form->p2_y,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  QObject::connect(
      this->Form->p2_z,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  //
  QObject::connect(
      this->Form->res_x,
      SIGNAL(valueChanged(int)),
      this, SLOT(setModified()));
  QObject::connect(
      this->Form->res_y,
      SIGNAL(valueChanged(int)),
      this, SLOT(setModified()));
  //
  QObject::connect(
      this->Form->dx,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  QObject::connect(
      this->Form->dy,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  //
  QObject::connect(
      this->Form->immediateMode,
      SIGNAL(stateChanged(int)),
      this, SLOT(setModified()));

  // These configure UI for constraints.
  QObject::connect(
      this->Form->constraintNone,
      SIGNAL(clicked()),
      this, SLOT(ApplyConstraint()));
  //
  QObject::connect(
      this->Form->constraintXy,
      SIGNAL(clicked()),
      this, SLOT(ApplyConstraint()));
  //
  QObject::connect(
      this->Form->constraintXz,
      SIGNAL(clicked()),
      this, SLOT(ApplyConstraint()));
  //
  QObject::connect(
      this->Form->constraintYz,
      SIGNAL(clicked()),
      this, SLOT(ApplyConstraint()));

  // These make sure constrained values get updated in the UI.
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

  pqNamedObjectPanel::linkServerManagerProperties();
}

//-----------------------------------------------------------------------------
pqSQPlaneSource::~pqSQPlaneSource()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::~pqSQPlaneSource" << endl;
  #endif

  delete this->Form;
  /*
  this->VTKConnect->Delete();
  this->VTKConnect=0;*/
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::contextMenuEvent(QContextMenuEvent *event)
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::contextMenuEvent" << endl;
  #endif

  QMenu context(this);

  QAction *copyAct=new QAction(tr("Copy Configuration"),&context);
  connect(copyAct, SIGNAL(triggered()), this, SLOT(CopyConfiguration()));
  context.addAction(copyAct);

  QAction *pasteAct=new QAction(tr("Paste Configuration"),&context);
  connect(pasteAct, SIGNAL(triggered()), this, SLOT(PasteConfiguration()));
  context.addAction(pasteAct);

  context.exec(event->globalPos());
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::CopyConfiguration()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::CopyConfiguration" << endl;
  #endif

  // grab the current configuration.
  ostringstream os;

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
  cerr << ":::::::::::::::::::::::::::::::PasteConfiguration" << endl;
  #endif

  QClipboard *clipboard=QApplication::clipboard();
  QString config=clipboard->text();

  if (!config.isEmpty())
    {
    vtkSmartPointer<vtkPVXMLParser> parser=vtkSmartPointer<vtkPVXMLParser>::New();
    parser->InitializeParser();
    parser->ParseChunk(config.toAscii().data(),static_cast<unsigned int>(config.size()));
    parser->CleanupParser();

    vtkPVXMLElement *xmlStream=parser->GetRootElement();
    if (!xmlStream)
      {
      sqErrorMacro(qDebug(),"Invalid SQPlaneSource configuration  pasted.");
      return;
      }

    vtkSmartPointer<vtkSQPlaneSourceConfigurationReader> reader
      = vtkSmartPointer<vtkSQPlaneSourceConfigurationReader>::New();

    reader->SetProxy(this->proxy());
    int ok=reader->ReadConfiguration(xmlStream);
    if (!ok)
      {
      sqErrorMacro(qDebug(),"Invalid SQPlaneSource configuration  hierarchy.");
      return;
      }

    this->PullServerConfig();
    }
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::Restore()
{
  QSettings settings("SciberQuest", "SciVisToolKit");
  QString lastUsedDir=settings.value("SQPlaneSource/lastUsedDir","").toString();

  QString fn=QFileDialog::getOpenFileName(this,"Open SQ Plane Source",lastUsedDir,"*.sqps");
  if (fn.size())
    {
    ifstream f(fn.toStdString().c_str(),ios_base::in);
    if (f.is_open())
      {
      char buf[1024];
      f.getline(buf,1024);
      if (string(buf).find("SQ Plane Source")!=string::npos)
        {
        // DescriptiveName
        f.getline(buf,1024);
        f.getline(buf,1024);
        this->Form->name->setText(buf);
        // Origin
        f.getline(buf,1024);
        f.getline(buf,1024);
        istringstream *is;
        is=new istringstream(buf);
        double p[3];
        *is >> p[0] >> p[1] >> p[2];
        delete is;
        this->Form->o_x->setText(QString("%1").arg(p[0]));
        this->Form->o_y->setText(QString("%1").arg(p[1]));
        this->Form->o_z->setText(QString("%1").arg(p[2]));
        // Point1
        f.getline(buf,1024);
        f.getline(buf,1024);
        is=new istringstream(buf);
        *is >> p[0] >> p[1] >> p[2];
        delete is;
        this->Form->p1_x->setText(QString("%1").arg(p[0]));
        this->Form->p1_y->setText(QString("%1").arg(p[1]));
        this->Form->p1_z->setText(QString("%1").arg(p[2]));
        // Point2
        f.getline(buf,1024);
        f.getline(buf,1024);
        is=new istringstream(buf);
        *is >> p[0] >> p[1] >> p[2];
        delete is;
        this->Form->p2_x->setText(QString("%1").arg(p[0]));
        this->Form->p2_y->setText(QString("%1").arg(p[1]));
        this->Form->p2_z->setText(QString("%1").arg(p[2]));
        // Resolution
        f.getline(buf,1024);
        f.getline(buf,1024);
        is=new istringstream(buf);
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
  cerr << ":::::::::::::::::::::::::::::::Save" << endl;
  #endif

  QString fn=QFileDialog::getSaveFileName(this,"Save SQ Plane Source","","*.sqps");
  if (fn.size())
    {
    QString lastUsedDir(StripFileNameFromPath(fn.toStdString()).c_str());
    QSettings settings("SciberQuest", "SciVisToolKit");
    settings.setValue("SQPlaneSource/lastUsedDir",lastUsedDir);

    ofstream f(fn.toStdString().c_str(),ios_base::out|ios_base::trunc);
    if (f.is_open())
      {
      f << "SQ Plane Source 1.0" << endl
        << "DescriptiveName" << endl
        << this->Form->name->text().toStdString() << endl
        << "Origin" << endl
        << this->Form->o_x->text().toDouble() << " "
        << this->Form->o_y->text().toDouble() << " "
        << this->Form->o_z->text().toDouble() << endl
        << "Point1" << endl
        << this->Form->p1_x->text().toDouble() << " "
        << this->Form->p1_y->text().toDouble() << " "
        << this->Form->p1_z->text().toDouble() << endl
        << "Point2" << endl
        << this->Form->p2_x->text().toDouble() << " "
        << this->Form->p2_y->text().toDouble() << " "
        << this->Form->p2_z->text().toDouble() << endl
        << "Resolution" << endl
        << this->Form->res_x->value() << " "
        << this->Form->res_y->value() << endl
        << endl;
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
      sqErrorMacro(qDebug(),"Failed to load the plane source configuration.");
      }
    }

  reader->Delete();

  this->PullServerConfig();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::saveConfiguration()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::saveConfiguration" << endl;
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
      sqErrorMacro(qDebug(),"Failed to save the plane source configuration.");
      }
    }

  writer->Delete();
}

//-----------------------------------------------------------------------------
int pqSQPlaneSource::ValidateCoordinates()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::ValidateCoordinates" << endl;
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
  cerr << ":::::::::::::::::::::::::::::::CaclulateNormal" << endl;
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

  vtkMath::Cross(v1,v2,n);
  int ok=vtkMath::Normalize(n);
  if (!ok)
    {
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
int pqSQPlaneSource::GetConstraint()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::GetConstraint" << endl;
  #endif

  if (this->Form->constraintNone->isChecked())
    {
    return SQPS_CONSTRAINT_NONE;
    }
  else
  if (this->Form->constraintXy->isChecked())
    {
    return SQPS_CONSTRAINT_XY;
    }
  else
  if (this->Form->constraintXz->isChecked())
    {
    return SQPS_CONSTRAINT_XZ;
    }
  else
  if (this->Form->constraintYz->isChecked())
    {
    return SQPS_CONSTRAINT_YZ;
    }

  sqErrorMacro(qDebug(),"Invalid constraint.");
  return SQPS_CONSTRAINT_INVALID;
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SetConstraint(int type)
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::SetConstraint" << endl;
  #endif

  switch(type)
    {
    case SQPS_CONSTRAINT_NONE:
      this->Form->constraintNone->click();
      break;

    case SQPS_CONSTRAINT_XY:
      this->Form->constraintXy->click();
      break;

    case SQPS_CONSTRAINT_XZ:
      this->Form->constraintXz->click();
      break;

    case SQPS_CONSTRAINT_YZ:
      this->Form->constraintYz->click();
      break;

    default:
      sqErrorMacro(qDebug(),"Invalid constraint " << type << ".");
      break;
    }
  this->ApplyConstraint();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::ApplyConstraint()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::ApplyConstraint" << endl;
  #endif

  int type=this->GetConstraint();

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
      sqErrorMacro(qDebug(),"Invalid constraint " << type << ".");
      break;
    }

    this->setModified();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::DimensionsModified()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::DimensionsModified" << endl;
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
  this->SpacingModified();

  return;
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SpacingModified()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::SpacingModified" << endl;
  #endif

  if (this->Form->spacingLock->isChecked())
    {
    // retreive the requested spacing.
    this->GetSpacing(this->Dx);

    // enforce uniform pixel aspect ratio
    if (this->Form->aspectLock->isChecked())
      {
      this->Dx[1]=this->Dx[0];
      this->Form->dy->setText(QString("%1").arg(this->Dx[0]));
      }

    // update the grid resolution to match the requested grid spacing
    // as closely as possible.
    this->Nx[0]=ceil(this->Dims[0]/this->Dx[0]);
    this->Nx[1]=ceil(this->Dims[1]/this->Dx[1]);
    this->SetResolution(this->Nx);
    }
  else
    {
    // update the spacing to match the requested resolution as closely
    // as possible.
    this->GetResolution(this->Nx);

    if (this->Form->aspectLock->isChecked())
      {
      this->Nx[1]=ceil(this->Nx[0]*this->Dims[1]/this->Dims[0]);
      }

    this->SetResolution(this->Nx);
    }

  // compute the new number of cells.
  int nCells=this->Nx[0]*this->Nx[1];
  this->Form->nCells->setText(QString("%1").arg(nCells));

  // update the spacing to match the actual spacing that will be used.
  this->Dx[0]=this->Dims[0]/this->Nx[0];
  this->Dx[1]=this->Dims[1]/this->Nx[1];
  this->SetSpacing(this->Dx);
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::ResolutionModified()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::ResolutionModified" << endl;
  #endif

  // retreive the requested resolution.
  this->GetResolution(this->Nx);

  // enforce a uniform pixel aspect ratio
  if (this->Form->aspectLock->isChecked())
    {
    this->Nx[1]=(this->Dims[0]>1E-6?this->Nx[0]*this->Dims[1]/this->Dims[0]:1);
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
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::SnapViewToNormal()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::SnapViewToNormal" << endl;
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
    sqErrorMacro(qDebug(),"Failed to get the current view.");
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
void pqSQPlaneSource::UpdateInformationEvent()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::UpdateInformationEvent" << endl;
  #endif
  // vtkSMProxy* pProxy=this->referenceProxy()->getProxy();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::PullServerConfig()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::PullServerConfig" << endl;
  #endif

  vtkSMProxy* pProxy=this->referenceProxy()->getProxy();

  // Name
  vtkSMStringVectorProperty *nameProp
    = dynamic_cast<vtkSMStringVectorProperty*>(pProxy->GetProperty("Name"));
  pProxy->UpdatePropertyInformation(nameProp);
  string name=nameProp->GetElement(0);
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

  // update derived/computed values.
  this->DimensionsModified();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::PushServerConfig()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::PushServerConfig" << endl;
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

  // Let proxy send updated values.
  pProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSQPlaneSource::accept()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::accept" << endl;
  #endif

  if (!this->ValidateCoordinates())
    {
    sqErrorMacro(qDebug(),"Invalid coordinate system.");
    }

  this->PushServerConfig();

  pqNamedObjectPanel::accept();
}



//-----------------------------------------------------------------------------
void pqSQPlaneSource::reset()
{
  #if defined pqSQPlaneSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::reset" << endl;
  #endif

  this->PullServerConfig();

  pqNamedObjectPanel::reset();
}

