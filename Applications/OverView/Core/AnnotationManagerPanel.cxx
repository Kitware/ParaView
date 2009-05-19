/*=========================================================================

   Program: ParaView
   Module:    AnnotationManagerPanel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "AnnotationManagerPanel.h"
#include "ui_AnnotationManagerPanel.h"

#include "AnnotationLink.h"

#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkAnnotationLink.h"
#include "vtkDataRepresentation.h"
#include "vtkEventQtSlotConnect.h"
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkInformationDoubleVectorKey.h>
#include <vtkInformationIntegerKey.h>
#include <vtkInformationVector.h>
#include "vtkProcessModule.h"
#include "vtkQtAnnotationView.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <QHeaderView>
#include <QItemDelegate>
#include <QMessageBox>
#include <QPointer>
#include <QScrollArea>
#include <QtDebug>
#include <QTimer>
#include <QVBoxLayout>

#include "pqSignalAdaptors.h"

////////////////////////////////////////////////////////////////////////////////////
// AnnotationManagerPanel::command

class AnnotationManagerPanel::command : public vtkCommand
{
public:
  static command* New(AnnotationManagerPanel& view)
  {
    return new command(view);
  }
  command(AnnotationManagerPanel& view) : Target(view) { }
  virtual void Execute(vtkObject*, unsigned long, void* layers)
  {
    Target.annotationChanged(reinterpret_cast<vtkAnnotationLayers*>(layers));
  }
  AnnotationManagerPanel& Target;
};

//////////////////////////////////////////////////////////////////////////////
// AnnotationManagerPanel::pqImplementation

struct AnnotationManagerPanel::pqImplementation : public Ui::AnnotationManagerPanel
{
public:
  pqImplementation() 
    {
    this->View = vtkSmartPointer<vtkQtAnnotationView>::New();
    this->Representation = vtkSmartPointer<vtkDataRepresentation>::New();
    this->Representation->SetAnnotationLink(AnnotationLink::instance().getLink());
    this->View->SetRepresentation(this->Representation);
    }

  ~pqImplementation()
    {
    }

  vtkSmartPointer<vtkQtAnnotationView> View;
  vtkSmartPointer<vtkDataRepresentation> Representation;
  pqSignalAdaptorColor* ColorAdapter;
};

/////////////////////////////////////////////////////////////////////////////////
// AnnotationManagerPanel

AnnotationManagerPanel::AnnotationManagerPanel(QWidget *p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  this->Command = command::New(*this);
  QVBoxLayout* vboxlayout = new QVBoxLayout(this);
  vboxlayout->setSpacing(0);
  vboxlayout->setMargin(0);
  vboxlayout->setObjectName("vboxLayout");

  QWidget* container = new QWidget(this);
  container->setObjectName("scrollWidget");
  container->setSizePolicy(QSizePolicy::MinimumExpanding,
    QSizePolicy::MinimumExpanding);

  QScrollArea* s = new QScrollArea(this);
  s->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setWidgetResizable(true);
  s->setObjectName("scrollArea");
  s->setFrameShape(QFrame::NoFrame);
  s->setWidget(container);
  vboxlayout->addWidget(s);

  this->Implementation->setupUi(container);
  this->setupGUI();
  
  this->Implementation->View->AddObserver(
    vtkCommand::AnnotationChangedEvent, this->Command);

  this->updateEnabledState();
}

//-----------------------------------------------------------------------------
AnnotationManagerPanel::~AnnotationManagerPanel()
{
  delete this->Implementation;
  this->Command->Delete();
}

//-----------------------------------------------------------------------------
void AnnotationManagerPanel::setupGUI()
{
  QVBoxLayout *layout = new QVBoxLayout(this->Implementation->viewFrame);
  layout->addWidget(this->Implementation->View->GetWidget());
  layout->setSpacing(0);
  layout->setMargin(0);
  layout->setObjectName("vboxLayout2");

  QObject::connect(this->Implementation->saveSelection, 
    SIGNAL(pressed()), this, SLOT(createAnnotationFromCurrentSelection()));

  this->Implementation->ColorAdapter = new pqSignalAdaptorColor(
    this->Implementation->color,
    "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)),
    false);

  QColor col(Qt::red);
  QList<QVariant> rgb;
  rgb.append(col.red() / 255.0);
  rgb.append(col.green() / 255.0);
  rgb.append(col.blue() / 255.0);
  this->Implementation->ColorAdapter->setColor(rgb);
}

//-----------------------------------------------------------------------------
void AnnotationManagerPanel::updateEnabledState()
{
}

void AnnotationManagerPanel::annotationChanged(vtkAnnotationLayers* a)
{
  AnnotationLink::instance().updateViews();
}

void AnnotationManagerPanel::createAnnotationFromCurrentSelection()
{
  vtkSelection* sel = AnnotationLink::instance().getLink()->GetCurrentSelection();
  if(!sel || sel->GetNumberOfNodes()==0)
    {
    return;
    }
  
  QString label = this->Implementation->label->text();
  if(label.isNull() || label.isEmpty())
    {
    return;
    }

  vtkSmartPointer<vtkAnnotation> a = vtkSmartPointer<vtkAnnotation>::New();
  vtkSmartPointer<vtkSelection> s = vtkSmartPointer<vtkSelection>::New();
  s->DeepCopy(sel);
  a->SetSelection(s);
  vtkInformation* ainfo = a->GetInformation();
  ainfo->Set(vtkAnnotation::ENABLED(), 0);
  ainfo->Set(vtkAnnotation::LABEL(),label.toAscii().data());
  QList<QVariant> rgba = this->Implementation->ColorAdapter->color().toList();
  double rgb[3];
  rgb[0] = rgba[0].toDouble();
  rgb[1] = rgba[1].toDouble();
  rgb[2] = rgba[2].toDouble();
  ainfo->Set(vtkAnnotation::COLOR(),rgb,3);
  vtkAnnotationLayers* annotations = this->Implementation->Representation->GetAnnotationLink()->GetAnnotationLayers();
  annotations->AddAnnotation(a);
  this->Implementation->Representation->Update();
  this->Implementation->View->Update();
  this->Implementation->label->clear();
}
