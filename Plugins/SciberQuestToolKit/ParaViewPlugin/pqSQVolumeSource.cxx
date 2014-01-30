/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#include "pqSQVolumeSource.h"

#include "vtkSQVolumeSourceConfigurationReader.h"
#include "vtkSQVolumeSourceConfigurationWriter.h"
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
#include <sstream>

// #define pqSQVolumeSourceDEBUG

//-----------------------------------------------------------------------------
pqSQVolumeSource::pqSQVolumeSource(
      pqProxy* l_proxy,
      QWidget* widget)
             :
      pqNamedObjectPanel(l_proxy, widget)
{
  #if defined pqSQVolumeSourceDEBUG
  std::cerr << ":::::pqSQVolumeSource::pqSQVolumeSource" << std::endl;
  #endif

  // Construct Qt form.
  this->Form=new pqSQVolumeSourceForm;
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
  this->Form->p3_x->setValidator(new QDoubleValidator(this->Form->p3_x));
  this->Form->p3_y->setValidator(new QDoubleValidator(this->Form->p3_y));
  this->Form->p3_z->setValidator(new QDoubleValidator(this->Form->p3_z));
  this->Form->dx->setValidator(new QDoubleValidator(this->Form->dx));
  this->Form->dy->setValidator(new QDoubleValidator(this->Form->dy));
  this->Form->dz->setValidator(new QDoubleValidator(this->Form->dy));

  // defaults
  this->Dims[0]=
  this->Dims[1]=
  this->Dims[2]=1.0;

  this->Dx[0]=
  this->Dx[1]=
  this->Dx[2]=1.0;

  this->Nx[0]=
  this->Nx[1]=
  this->Nx[2]=1;

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

  //
  QObject::connect(
      this->Form->p3_x,
      SIGNAL(editingFinished()),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->p3_y,
      SIGNAL(editingFinished()),
      this, SLOT(DimensionsModified()));
  QObject::connect(
      this->Form->p3_z,
      SIGNAL(editingFinished()),
      this, SLOT(DimensionsModified()));

  // link resolution to spacing
  QObject::connect(
      this->Form->res_x,
      SIGNAL(valueChanged(int)),
      this, SLOT(ResolutionModified()));
  QObject::connect(
      this->Form->res_y,
      SIGNAL(valueChanged(int)),
      this, SLOT(ResolutionModified()));
  QObject::connect(
      this->Form->res_z,
      SIGNAL(valueChanged(int)),
      this, SLOT(ResolutionModified()));

  // link spacing to resolution
  QObject::connect(
      this->Form->dx,
      SIGNAL(editingFinished()),
      this, SLOT(SpacingModified()));
  QObject::connect(
      this->Form->dy,
      SIGNAL(editingFinished()),
      this, SLOT(SpacingModified()));
  QObject::connect(
      this->Form->dz,
      SIGNAL(editingFinished()),
      this, SLOT(SpacingModified()));

  // make sure if uniform pixel aspect is changed the form is updated
  QObject::connect(
      this->Form->aspectLock,
      SIGNAL(toggled(bool)),
      this, SLOT(SpacingModified()));

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
      this->Form->p3_x,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Point3"),
      0);

  this->Links->addPropertyLink(
      this->Form->p3_y,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Point3"),
      1);

  this->Links->addPropertyLink(
      this->Form->p3_z,
      "text",
      SIGNAL(textChanged(QString)),
      pProxy,
      pProxy->GetProperty("Point3"),
      2);

  this->Links->addPropertyLink(
      this->Form->res_x,
      "value",
      SIGNAL(valueChanged(int)),
      pProxy,
      pProxy->GetProperty("Resolution"),
      0);

  this->Links->addPropertyLink(
      this->Form->res_y,
      "value",
      SIGNAL(valueChanged(int)),
      pProxy,
      pProxy->GetProperty("Resolution"),
      1);

  this->Links->addPropertyLink(
      this->Form->res_z,
      "value",
      SIGNAL(valueChanged(int)),
      pProxy,
      pProxy->GetProperty("Resolution"),
      2);

  this->Links->addPropertyLink(
      this->Form->immediateMode,
      "checked",
      SIGNAL(stateChanged(int)),
      pProxy,
      pProxy->GetProperty("ImmediateMode"));
}

//-----------------------------------------------------------------------------
pqSQVolumeSource::~pqSQVolumeSource()
{
  #if defined pqSQVolumeSourceDEBUG
  std::cerr << ":::::pqSQVolumeSource::~pqSQVolumeSource" << std::endl;
  #endif

  delete this->Form;
  delete this->Links;
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::contextMenuEvent(QContextMenuEvent *evnt)
{
  QMenu context(this);

  QAction *copyAct=new QAction(tr("Copy Configuration"),&context);
  connect(copyAct, SIGNAL(triggered()), this, SLOT(CopyConfiguration()));
  context.addAction(copyAct);

  QAction *pasteAct=new QAction(tr("Paste Configuration"),&context);
  connect(pasteAct, SIGNAL(triggered()), this, SLOT(PasteConfiguration()));
  context.addAction(pasteAct);

  context.exec(evnt->globalPos());
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::CopyConfiguration()
{
  // grab the current configuration.
  std::ostringstream os;

  vtkSQVolumeSourceConfigurationWriter *writer
    = vtkSQVolumeSourceConfigurationWriter::New();

  writer->SetProxy(this->proxy());
  writer->WriteConfiguration(os);

  // place it on the clipboard.
  QClipboard *clipboard=QApplication::clipboard();
  clipboard->setText(os.str().c_str());

  writer->Delete();
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::PasteConfiguration()
{
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
      pqSQErrorMacro(qDebug(),"Invalid SQVolumeSource configuration  pasted.");
      return;
      }

    vtkSmartPointer<vtkSQVolumeSourceConfigurationReader> reader
      = vtkSmartPointer<vtkSQVolumeSourceConfigurationReader>::New();

    reader->SetProxy(this->proxy());
    int ok=reader->ReadConfiguration(xmlStream);
    if (!ok)
      {
      pqSQErrorMacro(qDebug(),"Invalid SQVolumeSource configuration  hierarchy.");
      return;
      }

    this->PullServerConfig();
    }
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::loadConfiguration()
{
  vtkSQVolumeSourceConfigurationReader *reader
    = vtkSQVolumeSourceConfigurationReader::New();

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
void pqSQVolumeSource::saveConfiguration()
{
  #if defined pqSQVolumeSourceDEBUG
  std::cerr << ":::::pqSQVolumeSource::saveConfiguration" << std::endl;
  #endif

  vtkSQVolumeSourceConfigurationWriter *writer
    = vtkSQVolumeSourceConfigurationWriter::New();

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
void pqSQVolumeSource::GetOrigin(double *o)
{
  o[0]=this->Form->o_x->text().toDouble();
  o[1]=this->Form->o_y->text().toDouble();
  o[2]=this->Form->o_z->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::SetOrigin(double *o)
{
  this->Form->o_x->setText(QString("%1").arg(o[0]));
  this->Form->o_y->setText(QString("%1").arg(o[1]));
  this->Form->o_z->setText(QString("%1").arg(o[2]));
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::GetPoint1(double *p1)
{
  p1[0]=this->Form->p1_x->text().toDouble();
  p1[1]=this->Form->p1_y->text().toDouble();
  p1[2]=this->Form->p1_z->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::SetPoint1(double *p1)
{
  this->Form->p1_x->setText(QString("%1").arg(p1[0]));
  this->Form->p1_y->setText(QString("%1").arg(p1[1]));
  this->Form->p1_z->setText(QString("%1").arg(p1[2]));
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::GetPoint2(double *p2)
{
  p2[0]=this->Form->p2_x->text().toDouble();
  p2[1]=this->Form->p2_y->text().toDouble();
  p2[2]=this->Form->p2_z->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::SetPoint2(double *p2)
{
  this->Form->p2_x->setText(QString("%1").arg(p2[0]));
  this->Form->p2_y->setText(QString("%1").arg(p2[1]));
  this->Form->p2_z->setText(QString("%1").arg(p2[2]));
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::GetPoint3(double *p3)
{
  p3[0]=this->Form->p3_x->text().toDouble();
  p3[1]=this->Form->p3_y->text().toDouble();
  p3[2]=this->Form->p3_z->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::SetPoint3(double *p3)
{
  this->Form->p3_x->setText(QString("%1").arg(p3[0]));
  this->Form->p3_y->setText(QString("%1").arg(p3[1]));
  this->Form->p3_z->setText(QString("%1").arg(p3[2]));
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::GetResolution(int *res)
{
  res[0]=this->Form->res_x->text().toInt();
  res[1]=this->Form->res_y->text().toInt();
  res[2]=this->Form->res_z->text().toInt();
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::SetResolution(int *res)
{
  this->Form->res_x->blockSignals(true);
  this->Form->res_y->blockSignals(true);
  this->Form->res_z->blockSignals(true);
  this->Form->res_x->setValue(res[0]);
  this->Form->res_y->setValue(res[1]);
  this->Form->res_z->setValue(res[2]);
  this->Form->res_x->blockSignals(false);
  this->Form->res_y->blockSignals(false);
  this->Form->res_z->blockSignals(false);
  this->setModified();
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::GetSpacing(double *dx)
{
  dx[0]=this->Form->dx->text().toDouble();
  dx[1]=this->Form->dy->text().toDouble();
  dx[2]=this->Form->dz->text().toDouble();
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::SetSpacing(double *dx)
{
  this->Form->dx->blockSignals(true);
  this->Form->dy->blockSignals(true);
  this->Form->dz->blockSignals(true);
  this->Form->dx->setText(QString("%1").arg(dx[0]));
  this->Form->dy->setText(QString("%1").arg(dx[1]));
  this->Form->dz->setText(QString("%1").arg(dx[2]));
  this->Form->dx->blockSignals(false);
  this->Form->dy->blockSignals(false);
  this->Form->dz->blockSignals(false);
  this->setModified();
}

//-----------------------------------------------------------------------------
int pqSQVolumeSource::ValidateCoordinates()
{
  #if defined pqSQVolumeSourceDEBUG
  std::cerr << ":::::pqSQVolumeSource::ValidateCoordinates" << std::endl;
  #endif

  this->Form->coordStatus->setText("OK");
  this->Form->coordStatus->setStyleSheet("color:green; background-color:white;");

  double o[3];
  this->GetOrigin(o);

  double pts[9];
  this->GetPoint1(pts);
  this->GetPoint2(pts+3);
  this->GetPoint3(pts+6);

  double axes[9];
  axes[0]=pts[0]-o[0];
  axes[1]=pts[1]-o[1];
  axes[2]=pts[2]-o[2];
  axes[3]=pts[3]-o[0];
  axes[4]=pts[4]-o[1];
  axes[5]=pts[5]-o[2];
  axes[6]=pts[6]-o[0];
  axes[7]=pts[7]-o[1];
  axes[8]=pts[8]-o[2];

  int cases[6]={0,1,0,2,1,2};

  // check three cases
  for (int q=0; q<3; ++q)
    {
    int qq=2*q;

    double *a1=axes+3*cases[qq];
    double *a2=axes+3*cases[qq+1];

    double n[3];
    vtkMath::Cross(a1,a2,n);

    double norm=vtkMath::Normalize(n);
    if (norm<1.0e-6)
      {
      std::ostringstream os;
      os
        << "Error: A"
        << cases[qq]
        << " x A"
        << cases[qq+1]
        << "=0";

      this->Form->coordStatus->setText(os.str().c_str());
      this->Form->coordStatus->setStyleSheet("color:red; background-color:lightyellow;");
      this->Form->nCells->setText("Error");
      this->Form->dim_x->setText("Error");
      this->Form->dim_y->setText("Error");
      this->Form->dim_z->setText("Error");

      return 0;
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::DimensionsModified()
{
  #if defined pqSQVolumeSourceDEBUG
  std::cerr << ":::::pqSQVolumeSource::DimensionsModified" << std::endl;
  #endif

  if (!this->ValidateCoordinates())
    {
    return;
    }

  double o[3];
  this->GetOrigin(o);

  double p1[3];
  this->GetPoint1(p1);

  double p2[3];
  this->GetPoint2(p2);

  double p3[3];
  this->GetPoint3(p3);

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

  l_x=p3[0]-o[0];
  l_y=p3[1]-o[1];
  l_z=p3[2]-o[2];
  dim=sqrt(l_x*l_x+l_y*l_y+l_z*l_z);
  this->Dims[2]=dim;

  this->Form->dim_x->setText(QString("%1").arg(this->Dims[0]));
  this->Form->dim_y->setText(QString("%1").arg(this->Dims[1]));
  this->Form->dim_z->setText(QString("%1").arg(this->Dims[2]));

  // recompute resolution based on the current grid spacing
  // settings.
  this->SpacingModified();

  return;
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::SpacingModified()
{
  #if defined pqSQVolumeSourceDEBUG
  std::cerr << ":::::pqSQVolumeSource::SpacingModified" << std::endl;
  #endif

  // retreive the requested spacing.
  this->GetSpacing(this->Dx);

  // enforce uniform pixel aspect ratio
  if (this->Form->aspectLock->isChecked())
    {
    this->Dx[2]=this->Dx[1]=this->Dx[0];
    this->SetSpacing(this->Dx);
    }

  // update the grid resolution to match the requested grid spacing
  // as closely as possible.
  this->Nx[0]=(int)ceil(this->Dims[0]/this->Dx[0]);
  this->Nx[1]=(int)ceil(this->Dims[1]/this->Dx[1]);
  this->Nx[2]=(int)ceil(this->Dims[2]/this->Dx[2]);

  this->SetResolution(this->Nx);

  // compute the new number of cells.
  int nCells=this->Nx[0]*this->Nx[1]*this->Nx[2];
  this->Form->nCells->setText(QString("%1").arg(nCells));

  this->Links->accept();
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::ResolutionModified()
{
  #if defined pqSQVolumeSourceDEBUG
  std::cerr << ":::::pqSQVolumeSource::ResolutionModified" << std::endl;
  #endif

  // retreive the requested resolution.
  this->GetResolution(this->Nx);

  // enforce a uniform pixel aspect ratio
  if (this->Form->aspectLock->isChecked())
    {
    this->Nx[1]=(int)(this->Dims[0]>1E-6?this->Nx[0]*this->Dims[1]/this->Dims[0]:1.0);
    this->Nx[1]=(this->Nx[1]<1?1:this->Nx[1]);
    this->Nx[2]=(int)(this->Dims[0]>1E-6?this->Nx[0]*this->Dims[2]/this->Dims[0]:1.0);
    this->Nx[2]=(this->Nx[2]<1?1:this->Nx[2]);
    this->SetResolution(this->Nx);
    }

  // compute the new spacing
  this->Dx[0]=this->Dims[0]/this->Nx[0];
  this->Dx[1]=this->Dims[1]/this->Nx[1];
  this->Dx[2]=this->Dims[2]/this->Nx[2];

  this->SetSpacing(this->Dx);

  // compute the new number of cells.
  int nCells=this->Nx[0]*this->Nx[1]*this->Nx[2];
  this->Form->nCells->setText(QString("%1").arg(nCells));

  this->Links->accept();
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::PullServerConfig()
{
  #if defined pqSQVolumeSourceDEBUG
  std::cerr << ":::::pqSQVolumeSource::PullServerConfig" << std::endl;
  #endif

  vtkSMProxy* pProxy=this->referenceProxy()->getProxy();

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

  // Point 3
  vtkSMDoubleVectorProperty *p3Prop
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("Point3"));
  pProxy->UpdatePropertyInformation(p3Prop);
  double *p3=p3Prop->GetElements();
  this->SetPoint3(p3);

  // Resolution
  vtkSMIntVectorProperty *resProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("Resolution"));
  pProxy->UpdatePropertyInformation(resProp);
  int *res=resProp->GetElements();
  this->SetResolution(res);

  // Mode
  vtkSMIntVectorProperty *modeProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("ImmediateMode"));
  pProxy->UpdatePropertyInformation(modeProp);
  this->Form->immediateMode->setChecked(modeProp->GetElement(0));
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::PushServerConfig()
{
  #if defined pqSQVolumeSourceDEBUG

  std::cerr << ":::::pqSQVolumeSource::PushServerConfig" << std::endl;
  #endif
  vtkSMProxy* pProxy=this->referenceProxy()->getProxy();

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

  // Point 3
  double p3[3];
  this->GetPoint3(p3);
  vtkSMDoubleVectorProperty *p3Prop
    = dynamic_cast<vtkSMDoubleVectorProperty*>(pProxy->GetProperty("Point3"));
  p3Prop->SetElements(p3,3);


  int res[3];
  this->GetResolution(res);
  vtkSMIntVectorProperty *resProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("Resolution"));
  resProp->SetElements(res);

  // Mode
  vtkSMIntVectorProperty *modeProp
    = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("ImmediateMode"));
  pProxy->UpdatePropertyInformation(modeProp);
  modeProp->SetElement(0,this->Form->immediateMode->isChecked()?1:0);

  // Let proxy send updated values.
  pProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::accept()
{
  #if defined pqSQVolumeSourceDEBUG
  std::cerr << ":::::pqSQVolumeSource::accept" << std::endl;
  #endif

  if (!this->ValidateCoordinates())
    {
    pqSQErrorMacro(qDebug(),"Invalid coordinate system.");
    }

  pqNamedObjectPanel::accept();
}

//-----------------------------------------------------------------------------
void pqSQVolumeSource::reset()
{
  #if defined pqSQVolumeSourceDEBUG
  std::cerr << ":::::pqSQVolumeSource::reset" << std::endl;
  #endif

  pqNamedObjectPanel::reset();
}
