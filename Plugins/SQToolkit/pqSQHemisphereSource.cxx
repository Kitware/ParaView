/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "pqSQHemisphereSource.h"
#include "vtkSQHemisphereSourceConfigurationReader.h"
#include "vtkSQHemisphereSourceConfigurationWriter.h"
#include "SQMacros.h"

#include "pqProxy.h"
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

#include <vector>
using vtkstd::vector;
#include <string>
using vtkstd::string;
#include <fstream>
using std::ofstream;
using std::ifstream;
using std::ios_base;
#include <sstream>
using std::ostringstream;
using std::istringstream;
#include <iostream>
using std::cerr;
using std::endl;


//-----------------------------------------------------------------------------
pqSQHemisphereSource::pqSQHemisphereSource(
      pqProxy* l_proxy,
      QWidget* l_parent)
             :
      pqNamedObjectPanel(l_proxy,l_parent)
{
  #if defined pqSQHemisphereSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQHemisphereSource::pqSQHemisphereSource" << endl;
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


//   vtkSMProxy* pProxy=this->referenceProxy()->getProxy();

//   // Connect to server side pipeline's UpdateInformation events.
//   // The server side will look for keys during request information.
//   this->VTKConnect=vtkEventQtSlotConnect::New();
//   this->VTKConnect->Connect(
//       pProxy,
//       vtkCommand::UpdateInformationEvent,
//       this, SLOT(PullServerConfig()));

  // Set up configuration viewer
  this->PullServerConfig();
  this->setModified();

  // set up save/restore buttons
  QObject::connect(this->Form->save,SIGNAL(clicked()),this,SLOT(saveConfiguration()));
  QObject::connect(this->Form->restore,SIGNAL(clicked()),this,SLOT(loadConfiguration()));

  // These connection let PV know that we have changed, and makes the apply 
  // button activated.
  QObject::connect(
      this->Form->c_x,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  QObject::connect(
      this->Form->c_y,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  QObject::connect(
      this->Form->c_z,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  //
  QObject::connect(
      this->Form->n_x,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  QObject::connect(
      this->Form->n_y,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  QObject::connect(
      this->Form->n_z,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  //
  QObject::connect(
      this->Form->r,
      SIGNAL(textChanged(QString)),
      this, SLOT(setModified()));
  //
  QObject::connect(
      this->Form->res,
      SIGNAL(valueChanged(int)),
      this, SLOT(setModified()));

  // Let the super class do the undocumented stuff that needs to hapen.
  pqNamedObjectPanel::linkServerManagerProperties();

}

//-----------------------------------------------------------------------------
pqSQHemisphereSource::~pqSQHemisphereSource()
{
  #if defined pqSQHemisphereSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQHemisphereSource::~pqSQHemisphereSource" << endl;
  #endif

  delete this->Form;

//   this->VTKConnect->Delete();
//   this->VTKConnect=0;
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::Restore()
{
  QSettings settings("SciberQuest", "SciVisToolKit");
  QString lastUsedDir=settings.value("SQHemisphereSource/lastUsedDir","").toString();

  QString fn=QFileDialog::getOpenFileName(this,"Open SQ Hemisphere Source",lastUsedDir,"*.sqhs");
  if (fn.size())
    {
    ifstream f(fn.toStdString().c_str(),ios_base::in);
    if (f.is_open())
      {
      char buf[1024];
      f.getline(buf,1024);
      if (string(buf).find("SQ Hemisphere Source")!=string::npos)
        {
        // Center
        f.getline(buf,1024);
        f.getline(buf,1024);
        istringstream *is;
        is=new istringstream(buf);
        double c[3];
        *is >> c[0] >> c[1] >> c[2];
        delete is;
        this->Form->c_x->setText(QString("%1").arg(c[0]));
        this->Form->c_y->setText(QString("%1").arg(c[1]));
        this->Form->c_z->setText(QString("%1").arg(c[2]));
        // North
        f.getline(buf,1024);
        f.getline(buf,1024);
        is=new istringstream(buf);
        double n[3];
        *is >> n[0] >> n[1] >> n[2];
        delete is;
        this->Form->n_x->setText(QString("%1").arg(n[0]));
        this->Form->n_y->setText(QString("%1").arg(n[1]));
        this->Form->n_z->setText(QString("%1").arg(n[2]));
        // Radius
        f.getline(buf,1024);
        f.getline(buf,1024);
        is=new istringstream(buf);
        double r;
        *is >> r;
        delete is;
        this->Form->r->setText(QString("%1").arg(r));
        // Resolution
        f.getline(buf,1024);
        f.getline(buf,1024);
        is=new istringstream(buf);
        int res;
        *is >> res;
        delete is;
        this->Form->res->setValue(res);
        }
      else
        {
        QMessageBox::warning(this,"Open SQ Hemisphere Source","Error: Bad format not a SQ plane source file.");
        }
      f.close();
      }
    else
      {
      QMessageBox::warning(this,"Save SQ Hemisphere Source","Error: Could not open the file.");
      }
    }
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::Save()
{
  QString fn=QFileDialog::getSaveFileName(this,"Save SQ Hemisphere Source","","*.sqhs");
  if (fn.size())
    {
    QString lastUsedDir(StripFileNameFromPath(fn.toStdString()).c_str());
    QSettings settings("SciberQuest", "SciVisToolKit");
    settings.setValue("SQHemisphereSource/lastUsedDir",lastUsedDir);

    ofstream f(fn.toStdString().c_str(),ios_base::out|ios_base::trunc);
    if (f.is_open())
      {
      f << "SQ Hemisphere Source 1.0" << endl
        << "Center" << endl
        << this->Form->c_x->text().toDouble() << " "
        << this->Form->c_y->text().toDouble() << " "
        << this->Form->c_z->text().toDouble() << endl
        << "North" << endl
        << this->Form->n_x->text().toDouble() << " "
        << this->Form->n_y->text().toDouble() << " "
        << this->Form->n_z->text().toDouble() << endl
        << "Radius" << endl
        << this->Form->r->text().toDouble() << endl
        << "Resolution" << endl
        << this->Form->res->value() << endl
        << endl;
      f.close();
      }
    else
      {
      QMessageBox::warning(this,"Save SQ Hemisphere Source","Error: Failed to create the file.");
      }
    }
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::loadConfiguration()
{
  vtkSQHemisphereSourceConfigurationReader *reader=vtkSQHemisphereSourceConfigurationReader::New();
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
      sqErrorMacro(qDebug(),"Failed to load the hemisphere source configuration.");
      }
    }

  reader->Delete();

  this->PullServerConfig();
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::saveConfiguration()
{
  #if defined pqSQHemisphereSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQHemisphereSource::saveConfiguration" << endl;
  #endif

  vtkSQHemisphereSourceConfigurationWriter *writer=vtkSQHemisphereSourceConfigurationWriter::New();
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
      sqErrorMacro(qDebug(),"Failed to save the hemisphere source configuration.");
      }
    }

  writer->Delete();
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::UpdateInformationEvent()
{
  #if defined pqSQHemisphereSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQHemisphereSource::UpdateInformationEvent" << endl;
  #endif

  this->PullServerConfig();
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::PullServerConfig()
{
  #if defined pqSQHemisphereSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQHemisphereSource::PullServerConfig" << endl;
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
  cerr << "Pulled: " << endl
       << "C   " << c[0] << ", " << c[1] << ", " << c[2] << endl
       << "N   " << n[0] << ", " << n[1] << ", " << n[2] << endl
       << "r   " << r << endl
       << "res " << res << endl
       << endl;
  #endif
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::PushServerConfig()
{
  #if defined pqSQHemisphereSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQHemisphereSource::PushServerConfig" << endl;
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
  cerr << "Pushed: " << endl
       << "C   " << c[0] << ", " << c[1] << ", " << c[2] << endl
       << "N   " << n[0] << ", " << n[1] << ", " << n[2] << endl
       << "r   " << r << endl
       << "res " << res << endl
       << endl;
  #endif

  // Let proxy send updated values.
  pProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSQHemisphereSource::accept()
{
  #if defined pqSQHemisphereSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQHemisphereSource::accept" << endl;
  #endif

  this->PushServerConfig();

  // Let our superclass do the undocumented stuff that needs to be done.
  pqNamedObjectPanel::accept();
}


//-----------------------------------------------------------------------------
void pqSQHemisphereSource::reset()
{
  #if defined pqSQHemisphereSourceDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQHemisphereSource::reset" << endl;
  #endif

  this->PullServerConfig();

  // Let our superclass do the undocumented stuff that needs to be done.
  pqNamedObjectPanel::accept();
}

/// VTK stuffs
//   // Connect to server side pipeline's UpdateInformation events.
//   this->VTKConnect=vtkEventQtSlotConnect::New();
//   this->VTKConnect->Connect(
//       dbbProxy,
//       vtkCommand::UpdateInformationEvent,
//       this, SLOT(UpdateInformationEvent()));
//   // Get our initial state from the server side. In server side RequestInformation
//   // the database view is encoded in vtkInformationObject. We are relying on the 
//   // fact that there is a pending event waiting for us.
//   this->UpdateInformationEvent();

// 
//   // These connection let PV know that we have changed, and makes the apply button
//   // is activated.
//   QObject::connect(
//       this->Form->DatabaseView,
//       SIGNAL(itemChanged(QTreeWidgetItem*, int)),
//       this, SLOT(setModified()));

// vtkSMProxy* dbbProxy=this->referenceProxy()->getProxy();
/// Example of how to get stuff from the server side.
// vtkSMIntVectorProperty *serverDVMTimeProp
//   =dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("DatabaseViewMTime"));
// dbbProxy->UpdatePropertyInformation(serverDVMTimeProp);
// const int *serverDVMTime=dvmtProp->GetElement(0);
/// Example of how to get stuff from the XML configuration.
// vtkSMStringVectorProperty *ppProp
//   =dynamic_cast<vtkSMStringVectorProperty *>(dbbProxy->GetProperty("PluginPath"));
// const char *pp=ppProp->GetElement(0);
// this->Form->PluginPath->setText(pp);
/// Imediate update example.
// These are bad because it gets updated more than when you call
// modified, see the PV guide.
//  iuiProp->SetImmediateUpdate(1); set this in constructor
// vtkSMProperty *iuiProp=dbbProxy->GetProperty("InitializeUI");
// iuiProp->Modified();
/// Do some changes and force a push.
// vtkSMIntVectorProperty *meshProp
//   = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("PushMeshId"));
// meshProp->SetElement(meshIdx,meshId);
// ++meshIdx;
// dbbProxy->UpdateProperty("PushMeshId");
/// Catch all, update anything else that may need updating.
// dbbProxy->UpdateVTKObjects();
/// How a wiget in custom panel tells PV that things need to be applied
// QObject::connect(
//     this->Form->DatabaseView,
//     SIGNAL(itemChanged(QTreeWidgetItem*, int)),
//     this, SLOT(setModified()));
  // Pull run time configuration from server. The values are transfered
  // in the form of an ascii stream.
//   vtkSMIntVectorProperty *pidProp
//     = dynamic_cast<vtkSMIntVectorProperty*>(pProxy->GetProperty("Pid"));
//   pProxy->UpdatePropertyInformation(pidProp);
//   pid_t p=pidProp->GetElement(0);
//   cerr << "PID=" << p << endl;
/*
  vtkSMProxy* reader = this->referenceProxy()->getProxy();
  reader->UpdatePropertyInformation(reader->GetProperty("Pid"));
  int stamp = vtkSMPropertyHelper(reader, "Pid").GetAsInt();
  cerr << stamp;*/




  /// NOTE how to get something from the server.
  /// dbbProxy->UpdatePropertyInformation(reader->GetProperty("SILUpdateStamp"));
  /// int stamp = vtkSMPropertyHelper(dbbProxy, "SILUpdateStamp").GetAsInt();