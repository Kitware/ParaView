#include "TableToGraphPanel.h"

#include <pqComboBoxDomain.h>
#include <pqProxy.h>
#include <pqPropertyHelper.h>

#include <vtkSMProxy.h>

#include <QLabel>
#include <QLayout>
#include <QMessageBox>

#include <iostream>
#include <set>

TableToGraphPanel::TableToGraphPanel(pqProxy* proxy, QWidget* p) :
  pqObjectPanel(proxy, p)
{
  this->Widgets.setupUi(this);

  vtkSMProxy* const table_to_graph = proxy->getProxy();

  pqComboBoxDomain* const vertex_field_domain_1 = new pqComboBoxDomain(
    this->Widgets.vertexField1,
    table_to_graph->GetProperty("VertexField1"),
    "array_list");

  pqComboBoxDomain* const vertex_field_domain_2 = new pqComboBoxDomain(
    this->Widgets.vertexField2,
    table_to_graph->GetProperty("VertexField2"),
    "array_list");

  pqComboBoxDomain* const vertex_field_domain_3 = new pqComboBoxDomain(
    this->Widgets.vertexField3,
    table_to_graph->GetProperty("VertexField3"),
    "array_list");

  pqComboBoxDomain* const vertex_field_domain_4 = new pqComboBoxDomain(
    this->Widgets.vertexField4,
    table_to_graph->GetProperty("VertexField4"),
    "array_list");

  pqComboBoxDomain* const vertex_field_domain_5 = new pqComboBoxDomain(
    this->Widgets.vertexField5,
    table_to_graph->GetProperty("VertexField5"),
    "array_list");

  this->Widgets.vertexDomain1->setText(this->Widgets.vertexField1->currentText());
  this->Widgets.vertexDomain2->setText(this->Widgets.vertexField2->currentText());
  this->Widgets.vertexDomain3->setText(this->Widgets.vertexField3->currentText());
  this->Widgets.vertexDomain4->setText(this->Widgets.vertexField4->currentText());
  this->Widgets.vertexDomain5->setText(this->Widgets.vertexField5->currentText());

  const QStringList link_vertices = pqPropertyHelper(table_to_graph, "LinkVertices").GetAsStringList();
  if(link_vertices.size() > 2)
    {
    this->Widgets.vertexField1->setCurrentIndex(this->Widgets.vertexField1->findText(link_vertices[0]));
    this->Widgets.vertexDomain1->setText(link_vertices[1]);
    this->Widgets.vertexHidden1->setChecked(link_vertices[2] == "1");
    }
  if(link_vertices.size() > 5)
    {
    this->Widgets.vertexField2->setCurrentIndex(this->Widgets.vertexField1->findText(link_vertices[3]));
    this->Widgets.vertexDomain2->setText(link_vertices[4]);
    this->Widgets.vertexHidden2->setChecked(link_vertices[5] == "1");
    }
  if(link_vertices.size() > 8)
    {
    this->Widgets.vertexField3->setCurrentIndex(this->Widgets.vertexField1->findText(link_vertices[6]));
    this->Widgets.vertexDomain3->setText(link_vertices[7]);
    this->Widgets.vertexHidden3->setChecked(link_vertices[8] == "1");
    }
  if(link_vertices.size() > 11)
    {
    this->Widgets.vertexField4->setCurrentIndex(this->Widgets.vertexField1->findText(link_vertices[9]));
    this->Widgets.vertexDomain4->setText(link_vertices[10]);
    this->Widgets.vertexHidden4->setChecked(link_vertices[11] == "1");
    }
  if(link_vertices.size() > 14)
    {
    this->Widgets.vertexField5->setCurrentIndex(this->Widgets.vertexField1->findText(link_vertices[12]));
    this->Widgets.vertexDomain5->setText(link_vertices[13]);
    this->Widgets.vertexHidden5->setChecked(link_vertices[14] == "1");
    }

  const QStringList link_edges = pqPropertyHelper(table_to_graph, "LinkEdges").GetAsStringList();
  if(link_edges.size() > 1)
    {
    this->Widgets.edgeSource1->setCurrentIndex(this->Widgets.edgeSource1->findText(link_edges[0]));
    this->Widgets.edgeTarget1->setCurrentIndex(this->Widgets.edgeTarget1->findText(link_edges[1]));
    }
  if(link_edges.size() > 3)
    {
    this->Widgets.edgeSource2->setCurrentIndex(this->Widgets.edgeSource2->findText(link_edges[2]));
    this->Widgets.edgeTarget2->setCurrentIndex(this->Widgets.edgeTarget2->findText(link_edges[3]));
    }
  if(link_edges.size() > 5)
    {
    this->Widgets.edgeSource3->setCurrentIndex(this->Widgets.edgeSource3->findText(link_edges[4]));
    this->Widgets.edgeTarget3->setCurrentIndex(this->Widgets.edgeTarget3->findText(link_edges[5]));
    }
  if(link_edges.size() > 7)
    {
    this->Widgets.edgeSource4->setCurrentIndex(this->Widgets.edgeSource4->findText(link_edges[6]));
    this->Widgets.edgeTarget4->setCurrentIndex(this->Widgets.edgeTarget4->findText(link_edges[7]));
    }
  if(link_edges.size() > 9)
    {
    this->Widgets.edgeSource5->setCurrentIndex(this->Widgets.edgeSource5->findText(link_edges[8]));
    this->Widgets.edgeTarget5->setCurrentIndex(this->Widgets.edgeTarget5->findText(link_edges[9]));
    }


  QObject::connect(this->Widgets.enableVertex1, SIGNAL(toggled(bool)), this->Widgets.vertexField1, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableVertex1, SIGNAL(toggled(bool)), this->Widgets.vertexDomain1, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableVertex1, SIGNAL(toggled(bool)), this->Widgets.vertexHidden1, SLOT(setEnabled(bool)));
  
  QObject::connect(this->Widgets.enableVertex2, SIGNAL(toggled(bool)), this->Widgets.vertexField2, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableVertex2, SIGNAL(toggled(bool)), this->Widgets.vertexDomain2, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableVertex2, SIGNAL(toggled(bool)), this->Widgets.vertexHidden2, SLOT(setEnabled(bool)));
  
  QObject::connect(this->Widgets.enableVertex3, SIGNAL(toggled(bool)), this->Widgets.vertexField3, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableVertex3, SIGNAL(toggled(bool)), this->Widgets.vertexDomain3, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableVertex3, SIGNAL(toggled(bool)), this->Widgets.vertexHidden3, SLOT(setEnabled(bool)));
  
  QObject::connect(this->Widgets.enableVertex4, SIGNAL(toggled(bool)), this->Widgets.vertexField4, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableVertex4, SIGNAL(toggled(bool)), this->Widgets.vertexDomain4, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableVertex4, SIGNAL(toggled(bool)), this->Widgets.vertexHidden4, SLOT(setEnabled(bool)));
  
  QObject::connect(this->Widgets.enableVertex5, SIGNAL(toggled(bool)), this->Widgets.vertexField5, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableVertex5, SIGNAL(toggled(bool)), this->Widgets.vertexDomain5, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableVertex5, SIGNAL(toggled(bool)), this->Widgets.vertexHidden5, SLOT(setEnabled(bool)));

  QObject::connect(this->Widgets.enableEdge1, SIGNAL(toggled(bool)), this->Widgets.edgeSource1, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableEdge1, SIGNAL(toggled(bool)), this->Widgets.edgeTarget1, SLOT(setEnabled(bool)));
  
  QObject::connect(this->Widgets.enableEdge2, SIGNAL(toggled(bool)), this->Widgets.edgeSource2, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableEdge2, SIGNAL(toggled(bool)), this->Widgets.edgeTarget2, SLOT(setEnabled(bool)));
  
  QObject::connect(this->Widgets.enableEdge3, SIGNAL(toggled(bool)), this->Widgets.edgeSource3, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableEdge3, SIGNAL(toggled(bool)), this->Widgets.edgeTarget3, SLOT(setEnabled(bool)));
  
  QObject::connect(this->Widgets.enableEdge4, SIGNAL(toggled(bool)), this->Widgets.edgeSource4, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableEdge4, SIGNAL(toggled(bool)), this->Widgets.edgeTarget4, SLOT(setEnabled(bool)));
  
  QObject::connect(this->Widgets.enableEdge5, SIGNAL(toggled(bool)), this->Widgets.edgeSource5, SLOT(setEnabled(bool)));
  QObject::connect(this->Widgets.enableEdge5, SIGNAL(toggled(bool)), this->Widgets.edgeTarget5, SLOT(setEnabled(bool)));

  QObject::connect(this->Widgets.enableVertex1, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableVertex2, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableVertex3, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableVertex4, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableVertex5, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableEdge1, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableEdge2, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableEdge3, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableEdge4, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.enableEdge5, SIGNAL(toggled(bool)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.vertexField1, SIGNAL(currentIndexChanged(const QString&)), this->Widgets.vertexDomain1, SLOT(setText(const QString&)));
  QObject::connect(this->Widgets.vertexField2, SIGNAL(currentIndexChanged(const QString&)), this->Widgets.vertexDomain2, SLOT(setText(const QString&)));
  QObject::connect(this->Widgets.vertexField3, SIGNAL(currentIndexChanged(const QString&)), this->Widgets.vertexDomain3, SLOT(setText(const QString&)));
  QObject::connect(this->Widgets.vertexField4, SIGNAL(currentIndexChanged(const QString&)), this->Widgets.vertexDomain4, SLOT(setText(const QString&)));
  QObject::connect(this->Widgets.vertexField5, SIGNAL(currentIndexChanged(const QString&)), this->Widgets.vertexDomain5, SLOT(setText(const QString&)));

  QObject::connect(this->Widgets.vertexField1, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(onVertexTypeChanged()));
  QObject::connect(this->Widgets.vertexField2, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(onVertexTypeChanged()));
  QObject::connect(this->Widgets.vertexField3, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(onVertexTypeChanged()));
  QObject::connect(this->Widgets.vertexField4, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(onVertexTypeChanged()));
  QObject::connect(this->Widgets.vertexField5, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(onVertexTypeChanged()));

  QObject::connect(this->Widgets.enableVertex1, SIGNAL(toggled(bool)), this, SLOT(onVertexTypeChanged()));
  QObject::connect(this->Widgets.enableVertex2, SIGNAL(toggled(bool)), this, SLOT(onVertexTypeChanged()));
  QObject::connect(this->Widgets.enableVertex3, SIGNAL(toggled(bool)), this, SLOT(onVertexTypeChanged()));
  QObject::connect(this->Widgets.enableVertex4, SIGNAL(toggled(bool)), this, SLOT(onVertexTypeChanged()));
  QObject::connect(this->Widgets.enableVertex5, SIGNAL(toggled(bool)), this, SLOT(onVertexTypeChanged()));

  QObject::connect(this->Widgets.vertexField1, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexField2, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexField3, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexField4, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexField5, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.vertexDomain1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexDomain2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexDomain3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexDomain4, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexDomain5, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.vertexHidden1, SIGNAL(clicked(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexHidden2, SIGNAL(clicked(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexHidden3, SIGNAL(clicked(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexHidden4, SIGNAL(clicked(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexHidden5, SIGNAL(clicked(bool)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.edgeSource1, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeSource2, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeSource3, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeSource4, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeSource5, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.edgeTarget1, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeTarget2, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeTarget3, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeTarget4, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeTarget5, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));

  this->onVertexTypeChanged();
}

void TableToGraphPanel::onVertexTypeChanged()
{
  this->Widgets.edgeSource1->clear();
  this->Widgets.edgeSource2->clear();
  this->Widgets.edgeSource3->clear();
  this->Widgets.edgeSource4->clear();
  this->Widgets.edgeSource5->clear();
  this->Widgets.edgeTarget1->clear();
  this->Widgets.edgeTarget2->clear();
  this->Widgets.edgeTarget3->clear();
  this->Widgets.edgeTarget4->clear();
  this->Widgets.edgeTarget5->clear();
  
  QStringList types;
  if(this->Widgets.enableVertex1->isChecked())
    types << this->Widgets.vertexField1->currentText();
  if(this->Widgets.enableVertex2->isChecked())
    types << this->Widgets.vertexField2->currentText();
  if(this->Widgets.enableVertex3->isChecked())
    types << this->Widgets.vertexField3->currentText();
  if(this->Widgets.enableVertex4->isChecked())
    types << this->Widgets.vertexField4->currentText();
  if(this->Widgets.enableVertex5->isChecked())
    types << this->Widgets.vertexField5->currentText();

  this->Widgets.edgeSource1->addItems(types);
  this->Widgets.edgeSource2->addItems(types);
  this->Widgets.edgeSource3->addItems(types);
  this->Widgets.edgeSource4->addItems(types);
  this->Widgets.edgeSource5->addItems(types);
  this->Widgets.edgeTarget1->addItems(types);
  this->Widgets.edgeTarget2->addItems(types);
  this->Widgets.edgeTarget3->addItems(types);
  this->Widgets.edgeTarget4->addItems(types);
  this->Widgets.edgeTarget5->addItems(types);
}

void TableToGraphPanel::accept()
{

  QStringList link_vertices;
  if(this->Widgets.enableVertex1->isChecked())
    link_vertices << this->Widgets.vertexField1->currentText() << this->Widgets.vertexDomain1->text() << (this->Widgets.vertexHidden1->isChecked() ? "1" : "0");
  if(this->Widgets.enableVertex2->isChecked())
    link_vertices << this->Widgets.vertexField2->currentText() << this->Widgets.vertexDomain2->text() << (this->Widgets.vertexHidden2->isChecked() ? "1" : "0");
  if(this->Widgets.enableVertex3->isChecked())
    link_vertices << this->Widgets.vertexField3->currentText() << this->Widgets.vertexDomain3->text() << (this->Widgets.vertexHidden3->isChecked() ? "1" : "0");
  if(this->Widgets.enableVertex4->isChecked())
    link_vertices << this->Widgets.vertexField4->currentText() << this->Widgets.vertexDomain4->text() << (this->Widgets.vertexHidden4->isChecked() ? "1" : "0");
  if(this->Widgets.enableVertex5->isChecked())
    link_vertices << this->Widgets.vertexField5->currentText() << this->Widgets.vertexDomain5->text() << (this->Widgets.vertexHidden5->isChecked() ? "1" : "0");

  QStringList link_edges;
  if(this->Widgets.enableEdge1->isChecked())
    link_edges << this->Widgets.edgeSource1->currentText() << this->Widgets.edgeTarget1->currentText();
  if(this->Widgets.enableEdge2->isChecked())
    link_edges << this->Widgets.edgeSource2->currentText() << this->Widgets.edgeTarget2->currentText();
  if(this->Widgets.enableEdge3->isChecked())
    link_edges << this->Widgets.edgeSource3->currentText() << this->Widgets.edgeTarget3->currentText();
  if(this->Widgets.enableEdge4->isChecked())
    link_edges << this->Widgets.edgeSource4->currentText() << this->Widgets.edgeTarget4->currentText();
  if(this->Widgets.enableEdge5->isChecked())
    link_edges << this->Widgets.edgeSource5->currentText() << this->Widgets.edgeTarget5->currentText();

  vtkSMProxy* const table_to_graph = this->referenceProxy()->getProxy();

  pqPropertyHelper(table_to_graph, "LinkVertices").Set(link_vertices);
  pqPropertyHelper(table_to_graph, "LinkEdges").Set(link_edges);
  table_to_graph->UpdateVTKObjects();

  Superclass::accept();    
}

void TableToGraphPanel::reset()
{
  Superclass::reset();
}

