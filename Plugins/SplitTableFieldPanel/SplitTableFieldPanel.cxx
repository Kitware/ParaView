#include "SplitTableFieldPanel.h"

#include <pqComboBoxDomain.h>
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

  pqComboBoxDomain* const field_domain_1 = new pqComboBoxDomain(
    this->Widgets.fieldName1,
    split_table_field->GetProperty("Field1"),
    "array_list");

  pqComboBoxDomain* const field_domain_2 = new pqComboBoxDomain(
    this->Widgets.fieldName2,
    split_table_field->GetProperty("Field2"),
    "array_list");

  pqComboBoxDomain* const field_domain_3 = new pqComboBoxDomain(
    this->Widgets.fieldName3,
    split_table_field->GetProperty("Field3"),
    "array_list");

  pqComboBoxDomain* const field_domain_4 = new pqComboBoxDomain(
    this->Widgets.fieldName4,
    split_table_field->GetProperty("Field4"),
    "array_list");

  pqComboBoxDomain* const field_domain_5 = new pqComboBoxDomain(
    this->Widgets.fieldName5,
    split_table_field->GetProperty("Field5"),
    "array_list");

  pqComboBoxDomain* const field_domain_6 = new pqComboBoxDomain(
    this->Widgets.fieldName6,
    split_table_field->GetProperty("Field6"),
    "array_list");

  const QStringList fields = pqPropertyHelper(split_table_field, "Fields").GetAsStringList();
  if(fields.size() > 1)
    {
    this->Widgets.fieldName1->setCurrentIndex(this->Widgets.fieldName1->findText(fields[0]));
    this->Widgets.fieldDelimiter1->setText(fields[1]);
    }
  if(fields.size() > 3)
    {
    this->Widgets.fieldName2->setCurrentIndex(this->Widgets.fieldName2->findText(fields[2]));
    this->Widgets.fieldDelimiter2->setText(fields[3]);
    }
  if(fields.size() > 5)
    {
    this->Widgets.fieldName3->setCurrentIndex(this->Widgets.fieldName3->findText(fields[4]));
    this->Widgets.fieldDelimiter3->setText(fields[5]);
    }
  if(fields.size() > 7)
    {
    this->Widgets.fieldName4->setCurrentIndex(this->Widgets.fieldName4->findText(fields[6]));
    this->Widgets.fieldDelimiter4->setText(fields[7]);
    }
  if(fields.size() > 9)
    {
    this->Widgets.fieldName5->setCurrentIndex(this->Widgets.fieldName5->findText(fields[8]));
    this->Widgets.fieldDelimiter5->setText(fields[9]);
    }
  if(fields.size() > 11)
    {
    this->Widgets.fieldName6->setCurrentIndex(this->Widgets.fieldName6->findText(fields[10]));
    this->Widgets.fieldDelimiter6->setText(fields[11]);
    }

  QObject::connect(this->Widgets.enableField1, SIGNAL(toggled(bool)), this->Widgets.fieldName1, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableField1, SIGNAL(toggled(bool)), this->Widgets.fieldDelimiter1, SLOT(setEnabled(bool)));
  
  QObject::connect(this->Widgets.enableField2, SIGNAL(toggled(bool)), this->Widgets.fieldName2, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableField2, SIGNAL(toggled(bool)), this->Widgets.fieldDelimiter2, SLOT(setEnabled(bool)));
  
  QObject::connect(this->Widgets.enableField3, SIGNAL(toggled(bool)), this->Widgets.fieldName3, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableField3, SIGNAL(toggled(bool)), this->Widgets.fieldDelimiter3, SLOT(setEnabled(bool)));
  
  QObject::connect(this->Widgets.enableField4, SIGNAL(toggled(bool)), this->Widgets.fieldName4, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableField4, SIGNAL(toggled(bool)), this->Widgets.fieldDelimiter4, SLOT(setEnabled(bool)));
  
  QObject::connect(this->Widgets.enableField5, SIGNAL(toggled(bool)), this->Widgets.fieldName5, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableField5, SIGNAL(toggled(bool)), this->Widgets.fieldDelimiter5, SLOT(setEnabled(bool)));

  QObject::connect(this->Widgets.enableField6, SIGNAL(toggled(bool)), this->Widgets.fieldName6, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableField6, SIGNAL(toggled(bool)), this->Widgets.fieldDelimiter6, SLOT(setEnabled(bool)));

  QObject::connect(this->Widgets.enableField1, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableField2, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableField3, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableField4, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableField5, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableField6, SIGNAL(toggled(bool)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.fieldName1, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldName2, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldName3, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldName4, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldName5, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldName6, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.fieldDelimiter1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldDelimiter2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldDelimiter3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldDelimiter4, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldDelimiter5, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.fieldDelimiter6, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
}

void SplitTableFieldPanel::accept()
{
  QStringList fields;
  if(this->Widgets.enableField1->isChecked() && !this->Widgets.fieldDelimiter1->text().isEmpty())
    fields << this->Widgets.fieldName1->currentText() << this->Widgets.fieldDelimiter1->text();
  if(this->Widgets.enableField1->isChecked() && !this->Widgets.fieldDelimiter2->text().isEmpty())
    fields << this->Widgets.fieldName2->currentText() << this->Widgets.fieldDelimiter2->text();
  if(this->Widgets.enableField1->isChecked() && !this->Widgets.fieldDelimiter3->text().isEmpty())
    fields << this->Widgets.fieldName3->currentText() << this->Widgets.fieldDelimiter3->text();
  if(this->Widgets.enableField1->isChecked() && !this->Widgets.fieldDelimiter4->text().isEmpty())
    fields << this->Widgets.fieldName4->currentText() << this->Widgets.fieldDelimiter4->text();
  if(this->Widgets.enableField1->isChecked() && !this->Widgets.fieldDelimiter5->text().isEmpty())
    fields << this->Widgets.fieldName5->currentText() << this->Widgets.fieldDelimiter5->text();
  if(this->Widgets.enableField1->isChecked() && !this->Widgets.fieldDelimiter6->text().isEmpty())
    fields << this->Widgets.fieldName6->currentText() << this->Widgets.fieldDelimiter6->text();

  vtkSMProxy* const split_table_field = this->referenceProxy()->getProxy();
  pqPropertyHelper(split_table_field, "Fields").Set(fields);
  split_table_field->UpdateVTKObjects();
 
  Superclass::accept();    
}

void SplitTableFieldPanel::reset()
{
  Superclass::reset();
}

