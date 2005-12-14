

#include "pqOpenExodusOptions.h"
#include <QCheckBox>

#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMArraySelectionDomain.h>


pqOpenExodusOptions::pqOpenExodusOptions(vtkSMSourceProxy* exodusReader, QWidget* p)
  : QDialog(p), ExodusReader(exodusReader)
{
  this->setupUi(this);

  if(!this->ExodusReader)
    return;

  this->ExodusReader->UpdateVTKObjects();
  this->ExodusReader->UpdatePipelineInformation();
  
  int i;
  
  // get and populate block ids
  vtkSMStringVectorProperty* blockStatus = 
    vtkSMStringVectorProperty::SafeDownCast(this->ExodusReader->GetProperty("BlockArrayStatus"));
  vtkSMStringVectorProperty* blockInfo =
    vtkSMStringVectorProperty::SafeDownCast(this->ExodusReader->GetProperty("BlockArrayInfo"));
  vtkSMArraySelectionDomain* blockDomain = vtkSMArraySelectionDomain::SafeDownCast(blockStatus->GetDomain("array_list"));
    
  int numBlocks = blockDomain->GetNumberOfStrings();
  for(i=0; i<numBlocks; i++)
    {
    QCheckBox* cb = new QCheckBox(this->BlocksGroup);
    cb->setText(blockDomain->GetString(i));
    int tmp;
    QVariant checked = "0";
    int idx = blockInfo->GetElementIndex(blockDomain->GetString(i), tmp);
    if(tmp)
      {
      checked = blockInfo->GetElement(idx+1);
      }
    cb->setChecked(checked == "1" ? true : false);
    this->BlocksGroup->layout()->addWidget(cb);
    }

  // get and populate cell arrays
  vtkSMStringVectorProperty* cellArrayStatus = 
    vtkSMStringVectorProperty::SafeDownCast(this->ExodusReader->GetProperty("CellArrayStatus"));
  vtkSMStringVectorProperty* cellArrayInfo =
    vtkSMStringVectorProperty::SafeDownCast(this->ExodusReader->GetProperty("CellArrayInfo"));
  vtkSMArraySelectionDomain* cellArrayDomain = vtkSMArraySelectionDomain::SafeDownCast(cellArrayStatus->GetDomain("array_list"));
    
  int numCells = cellArrayDomain->GetNumberOfStrings();
  for(i=0; i<numCells; i++)
    {
    QCheckBox* cb = new QCheckBox(this->ElementVariablesGroup);
    cb->setText(cellArrayDomain->GetString(i));
    int tmp;
    QVariant checked = "0";
    int idx = cellArrayInfo->GetElementIndex(cellArrayDomain->GetString(i), tmp);
    if(tmp)
      {
      checked = cellArrayInfo->GetElement(idx+1);
      }
    cb->setChecked(checked == "1" ? true : false);
    this->ElementVariablesGroup->layout()->addWidget(cb);
    }

  // get and populate point arrays
  vtkSMStringVectorProperty* pointArrayStatus = 
    vtkSMStringVectorProperty::SafeDownCast(this->ExodusReader->GetProperty("PointArrayStatus"));
  vtkSMStringVectorProperty* pointArrayInfo =
    vtkSMStringVectorProperty::SafeDownCast(this->ExodusReader->GetProperty("PointArrayInfo"));
  vtkSMArraySelectionDomain* pointArrayDomain = vtkSMArraySelectionDomain::SafeDownCast(pointArrayStatus->GetDomain("array_list"));
    
  int numPoints = pointArrayDomain->GetNumberOfStrings();
  for(i=0; i<numPoints; i++)
    {
    QCheckBox* cb = new QCheckBox(this->NodeVariablesGroup);
    cb->setText(pointArrayDomain->GetString(i));
    int tmp;
    QVariant checked = "0";
    int idx = pointArrayInfo->GetElementIndex(pointArrayDomain->GetString(i), tmp);
    if(tmp)
      {
      checked = pointArrayInfo->GetElement(idx+1);
      }
    cb->setChecked(checked == "1" ? true : false);
    this->NodeVariablesGroup->layout()->addWidget(cb);
    }
  
  // get and populate side sets
  vtkSMStringVectorProperty* sideSetStatus = 
    vtkSMStringVectorProperty::SafeDownCast(this->ExodusReader->GetProperty("SideSetArrayStatus"));
  vtkSMStringVectorProperty* sideSetInfo =
    vtkSMStringVectorProperty::SafeDownCast(this->ExodusReader->GetProperty("SideSetInfo"));
  vtkSMArraySelectionDomain* sideSetDomain = vtkSMArraySelectionDomain::SafeDownCast(sideSetStatus->GetDomain("array_list"));
    
  int numSideSets = sideSetDomain->GetNumberOfStrings();
  for(i=0; i<numSideSets; i++)
    {
    QCheckBox* cb = new QCheckBox(this->SideSetGroup);
    cb->setText(sideSetDomain->GetString(i));
    int tmp;
    QVariant checked = "0";
    int idx = sideSetInfo->GetElementIndex(sideSetDomain->GetString(i), tmp);
    if(tmp)
      {
      checked = sideSetInfo->GetElement(idx+1);
      }
    cb->setChecked(checked == "1" ? true : false);
    this->SideSetGroup->layout()->addWidget(cb);
    }
  
  // get and populate node sets
  vtkSMStringVectorProperty* nodeSetStatus = 
    vtkSMStringVectorProperty::SafeDownCast(this->ExodusReader->GetProperty("NodeSetArrayStatus"));
  vtkSMStringVectorProperty* nodeSetInfo =
    vtkSMStringVectorProperty::SafeDownCast(this->ExodusReader->GetProperty("NodeSetInfo"));
  vtkSMArraySelectionDomain* nodeSetDomain = vtkSMArraySelectionDomain::SafeDownCast(nodeSetStatus->GetDomain("array_list"));
    
  int numNodeSets = nodeSetDomain->GetNumberOfStrings();
  for(i=0; i<numNodeSets; i++)
    {
    QCheckBox* cb = new QCheckBox(this->NodeSetsGroup);
    cb->setText(nodeSetDomain->GetString(i));
    int tmp;
    QVariant checked = "0";
    int idx = nodeSetInfo->GetElementIndex(nodeSetDomain->GetString(i), tmp);
    if(tmp)
      {
      checked = nodeSetInfo->GetElement(idx+1);
      }
    cb->setChecked(checked == "1" ? true : false);
    this->NodeSetsGroup->layout()->addWidget(cb);
    }
}

pqOpenExodusOptions::~pqOpenExodusOptions()
{
}

void pqOpenExodusOptions::accept()
{
  int i;

  // set blocks
  vtkSMStringVectorProperty* blockStatus = 
    vtkSMStringVectorProperty::SafeDownCast(this->ExodusReader->GetProperty("BlockArrayStatus"));
  vtkSMArraySelectionDomain* blockDomain = vtkSMArraySelectionDomain::SafeDownCast(blockStatus->GetDomain("array_list"));
  int numBlocks = blockDomain->GetNumberOfStrings();
  for(i=0; i<numBlocks; i++)
    {
    QLayout* l = this->BlocksGroup->layout();
    QCheckBox* cb = qobject_cast<QCheckBox*>(l->itemAt(l->count() - numBlocks + i)->widget());
    QString checked = cb->isChecked() ? "1" : "0";
    blockStatus->SetElement(0, cb->text().toAscii().data());
    blockStatus->SetElement(1, checked.toAscii().data());
    this->ExodusReader->UpdateVTKObjects();
    }

  // set element arrays
  vtkSMStringVectorProperty* cellArrayStatus = 
    vtkSMStringVectorProperty::SafeDownCast(this->ExodusReader->GetProperty("CellArrayStatus"));
  vtkSMArraySelectionDomain* cellArrayDomain = vtkSMArraySelectionDomain::SafeDownCast(cellArrayStatus->GetDomain("array_list"));
  int numCells = cellArrayDomain->GetNumberOfStrings();
  for(i=0; i<numCells; i++)
    {
    QLayout* l = this->ElementVariablesGroup->layout();
    QCheckBox* cb = qobject_cast<QCheckBox*>(l->itemAt(l->count() - numCells + i)->widget());
    QString checked = cb->isChecked() ? "1" : "0";
    cellArrayStatus->SetElement(0, cb->text().toAscii().data());
    cellArrayStatus->SetElement(1, checked.toAscii().data());
    this->ExodusReader->UpdateVTKObjects();
    }


  QDialog::accept();
}

