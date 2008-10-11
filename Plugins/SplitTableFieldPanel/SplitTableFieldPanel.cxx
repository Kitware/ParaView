#include "SplitTableFieldPanel.h"

#include <pqProxy.h>
#include <pqPropertyHelper.h>

#include <vtkSMProxy.h>

#include <QLabel>
#include <QLayout>
#include <QMessageBox>

#include <iostream>
#include <set>

SplitTableFieldPanel::SplitTableFieldPanel(pqProxy* proxy, QWidget* p) :
  pqObjectPanel(proxy, p)
{
  this->Widgets.setupUi(this);

  vtkSMProxy* const split_table_field = proxy->getProxy();

  const QStringList fields = pqPropertyHelper(split_table_field, "Fields").GetAsStringList();
  if(fields.size() > 1)
    {
    this->Widgets.fieldName1->setText(fields[0]);
    this->Widgets.fieldDelimiter1->setText(fields[1]);
    }
  if(fields.size() > 3)
    {
    this->Widgets.fieldName2->setText(fields[3]);
    this->Widgets.fieldDelimiter2->setText(fields[4]);
    }
  if(fields.size() > 5)
    {
    this->Widgets.fieldName3->setText(fields[6]);
    this->Widgets.fieldDelimiter3->setText(fields[7]);
    }

  QObject::connect(this->Widgets.fieldName1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldName2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldName3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.fieldDelimiter1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldDelimiter2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldDelimiter3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
}

void SplitTableFieldPanel::accept()
{
  QStringList fields;
  if(!this->Widgets.fieldName1->text().isEmpty() && !this->Widgets.fieldDelimiter1->text().isEmpty())
    fields << this->Widgets.fieldName1->text() << this->Widgets.fieldDelimiter1->text();
  if(!this->Widgets.fieldName2->text().isEmpty() && !this->Widgets.fieldDelimiter2->text().isEmpty())
    fields << this->Widgets.fieldName2->text() << this->Widgets.fieldDelimiter2->text();
  if(!this->Widgets.fieldName3->text().isEmpty() && !this->Widgets.fieldDelimiter3->text().isEmpty())
    fields << this->Widgets.fieldName3->text() << this->Widgets.fieldDelimiter3->text();

  vtkSMProxy* const split_table_field = this->referenceProxy()->getProxy();
  pqPropertyHelper(split_table_field, "Fields").Set(fields);
  split_table_field->UpdateVTKObjects();
 
  Superclass::accept();    
}

void SplitTableFieldPanel::reset()
{
  Superclass::reset();
}

