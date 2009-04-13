#include "pqVisItDatabaseBridgePanel.h"
#include "vtkVisItDatabaseBridgeTypes.h" 

#include "avtSILCollection.h"// for SIL_* enum

#include "pqProxy.h"
#include "pqTreeWidgetItemObject.h"
#include "pqTreeWidgetSelectionHelper.h"
#include "pqTreeWidgetCheckHelper.h"

#include "vtkSMProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMIntVectorProperty.h"


#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVSILInformation.h"
#include "vtkGraph.h"
#include "vtkAdjacentVertexIterator.h"
#include "vtkOutEdgeIterator.h"
#include "vtkInEdgeIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkStringArray.h"
#include "vtkIntArray.h"

#include <QTreeWidgetItem>
#include <QString>

#if defined vtkVisitDatabaseBridgeDEBUG
#include <PrintUtils.cxx>
#endif

#include <vtkstd/vector>
using vtkstd::vector;
#include <vtkstd/string>
using vtkstd::string;


namespace {
// Some static data.
static QPixmap pixmaps[] =
    {
    QPixmap(":/pqWidgets/Icons/pqNodalData16.png"),
    QPixmap(":/pqWidgets/Icons/pqCellCenterData16.png"),
    QPixmap(":/pqWidgets/Icons/pqCellCenterData16.png"),
    QPixmap(":/pqWidgets/Icons/pqFaceCenterData16.png"),
    QPixmap(":/pqWidgets/Icons/pqEdgeCenterData16.png"),
    QPixmap(":/pqWidgets/Icons/pqNodeSetData16.png"),
    QPixmap(":/pqWidgets/Icons/pqEdgeSetData16.png"),
    QPixmap(":/pqWidgets/Icons/pqFaceSetData16.png"),
    QPixmap(":/pqWidgets/Icons/pqSideSetData16.png"),
    QPixmap(":/pqWidgets/Icons/pqElemSetData16.png"),
    QPixmap(":/pqWidgets/Icons/pqNodeMapData16.png"),
    QPixmap(":/pqWidgets/Icons/pqEdgeMapData16.png"),
    QPixmap(":/pqWidgets/Icons/pqFaceMapData16.png"),
    QPixmap(":/pqWidgets/Icons/pqElemMapData16.png"),
    QPixmap(":/pqWidgets/Icons/pqGlobalData16.png")
    };

// Tree widget item data.
enum {
  QT_SS_ID=Qt::UserRole,
  QT_SS_ROLE,
  QT_SS_VERT
};

//*****************************************************************************
// Recursively traverse the SIL in the given tree adding each vertex
// to a qt tree widget.
void CopySIL(
      vtkGraph *dbView,
      vtkOutEdgeIterator * eit,
      QTreeWidgetItem *p,
      QTreeWidget *w,
      vector<QTreeWidgetItem *> &items)
{
  // Data for the view.
  // vert
  vtkStringArray *uiLabels
    = dynamic_cast<vtkStringArray *>(dbView->GetVertexData()->GetAbstractArray(0));
  vtkIntArray *ssIds
    = dynamic_cast<vtkIntArray *>(dbView->GetVertexData()->GetAbstractArray(1));
  vtkIntArray *ssRole
    = dynamic_cast<vtkIntArray *>(dbView->GetVertexData()->GetAbstractArray(2));
  // 
  vtkIntArray *edgeType
    = dynamic_cast<vtkIntArray *>(dbView->GetEdgeData()->GetAbstractArray(0));

  // Visit each vertex
  while (eit->HasNext())
    {
    vtkOutEdgeType edge=eit->Next();
    int vert=edge.Target;

    if (edgeType->GetValue(edge.Id)!=SIL_EDGE_STRUCTURAL)
      {
      // We have to skip over link edges.
      continue;
      }

    bool ok;
    // Use labels from the reader to identify entries in the 
    // Qtree.
    string uiLabel=uiLabels->GetValue(vert);
    QList<QString> label;
    label.append(uiLabel.c_str());

    QTreeWidgetItem *item;
    if (p)
      {
      // This is a branch (category) or leaf (SIL Set).
      item=new pqTreeWidgetItemObject(p,label);
      }
    else
      {
      // This is a top level item (mesh).
      item=new pqTreeWidgetItemObject(w,label);
      item->setData(0, Qt::DecorationRole, pixmaps[1]);
      // I'd like to make the check box on the mesh entries checkable
      // such that when clicked, all child domains,  all child 
      // assemblies and all child blocks are check/unchecked as well
      // and Checked/UnChecked/Partial state apply only to those branches
      // not to others such as arrays, expressions, or materials.
      }
    // Save the pointer to the widget for easy lookup. Want to store in the
    // database view but there is not mechanism for storing pointers.
    items[vert]=item;
    // Configure
    item->setFlags(item->flags() | Qt::ItemIsTristate);
    item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    item->setCheckState(0,Qt::Unchecked);
    item->setExpanded(false);
    // Save the vertex id so that we can look up various data stored in the view.
    item->setData(0,QT_SS_VERT,QVariant(vert));

    // Follow the edges out.
    vtkOutEdgeIterator *childEit=vtkOutEdgeIterator::New();
    dbView->GetOutEdges(vert,childEit);
    CopySIL(dbView,childEit,item,w,items);
    childEit->Delete();
    }
  return;
}

//******************************************************************************
// Recursively follow back links in the database view, so that
// collection dependencies are updated as well.
void UpdateCheckStateOut(
      vtkGraph *dbView,
      vector<QTreeWidgetItem *> &items,
      vtkIdType vert,
      Qt::CheckState state)
{
  vtkIntArray *edgeType
    = dynamic_cast<vtkIntArray *>(dbView->GetEdgeData()->GetAbstractArray(0));

  vtkOutEdgeIterator *it=vtkOutEdgeIterator::New();
  dbView->GetOutEdges(vert,it);

  // Follow links out.
  while (it->HasNext())
    {
    vtkOutEdgeType edge=it->Next();
    if (edgeType->GetValue(edge.Id)==SIL_EDGE_LINK)
      {
      UpdateCheckStateOut(dbView,items,edge.Target,state);

      QTreeWidgetItem *item=items[edge.Target];
      item->setData(0,Qt::CheckStateRole,state);

      #if defined vtkVisitDatabaseBridgeDEBUG
      cerr << "\tupdating out" << items[edge.Target]->text(0).toStdString();
      cerr << endl;
      #endif
      }
    }
  it->Delete();
}

//******************************************************************************
// Recursively follow back links in the database view, so that
// collection dependencies are updated as well.
void UpdateCheckStateIn(
      vtkGraph *dbView,
      vector<QTreeWidgetItem *> &items,
      vtkIdType vert,
      Qt::CheckState state)
{
  // If we are being unchecked then we want to examine
  // collections that link in, so that they will be unchecked.
  if (state!=Qt::Unchecked)
    {
    return;
    }

  vtkIntArray *edgeType
    = dynamic_cast<vtkIntArray *>(dbView->GetEdgeData()->GetAbstractArray(0));

  vtkInEdgeIterator *it=vtkInEdgeIterator::New();
  dbView->GetInEdges(vert,it);

  // Follow links in.
  while (it->HasNext())
    {
    vtkInEdgeType edge=it->Next();
    if (edgeType->GetValue(edge.Id)==SIL_EDGE_LINK)
      {
      UpdateCheckStateIn(dbView,items,edge.Source,state);

      QTreeWidgetItem *item=items[edge.Source];
      item->setData(0,Qt::CheckStateRole,state);

      #if defined vtkVisitDatabaseBridgeDEBUG
      cerr << "\tupdating in " << items[edge.Source]->text(0).toStdString();
      cerr << endl;
      #endif
      }
    }
  it->Delete();
}

};

//-----------------------------------------------------------------------------
pqVisItDatabaseBridgePanel::pqVisItDatabaseBridgePanel(
        pqProxy* proxy,
        QWidget* widget)
  : pqNamedObjectPanel(proxy, widget)
{
  #if defined vtkVisitDatabaseBridgeDEBUG
  cerr << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::pqVisItDatabaseBridgePanel" << endl;
  #endif

  // Construct Qt form.
  this->Form=new pqVisItDatabaseBridgeForm;
  this->Form->setupUi(this);

  this->DatabaseView=NULL;

  vtkSMProxy* dbbProxy=this->referenceProxy()->getProxy();

  // Various U.I. initialization.
  new pqTreeWidgetCheckHelper(this->Form->DatabaseView,0,this);

  // Pull some property values from the XML...
  // ...to display the plugin path...
  vtkSMStringVectorProperty *ppProp
    =dynamic_cast<vtkSMStringVectorProperty *>(dbbProxy->GetProperty("PluginPath"));
  const char *pp=ppProp->GetElement(0);
  this->Form->PluginPath->setText(pp);
  // ...and to display the plugin id.
  vtkSMStringVectorProperty *piProp
    =dynamic_cast<vtkSMStringVectorProperty *>(dbbProxy->GetProperty("PluginId"));
  const char *pi=piProp->GetElement(0);
  this->Form->PluginId->setText(pi);

  // Connect to server side pipeline's UpdateInformation events.
  this->VTKConnect=vtkEventQtSlotConnect::New();
  this->VTKConnect->Connect(
      dbbProxy,
      vtkCommand::UpdateInformationEvent,
      this, SLOT(UpdateInformationEvent()));
  // Get our initial state from the server side. In server side RequestInformation
  // the database view is encoded in vtkInformationObject. We are relying on the 
  // fact that there is a pending event waiting for us.
  this->ViewMTime=-1;
  this->UpdateInformationEvent();

  // Watch items in the tree widget, when the check state changes
  // we have to follow links in the SIL and update their state's.
  QObject::connect(
      this->Form->DatabaseView,
      SIGNAL(itemChanged(QTreeWidgetItem *, int)),
      this, SLOT(UpdateCheckState(QTreeWidgetItem*, int)));

  // These connection let PV know that we have changed, and makes the apply button
  // is activated.
  QObject::connect(
      this->Form->DatabaseView,
      SIGNAL(itemChanged(QTreeWidgetItem*, int)),
      this, SLOT(setModified()));

  // Let the super class do the undocumented stuff that needs to hapen.
  pqNamedObjectPanel::linkServerManagerProperties();
}



//-----------------------------------------------------------------------------
pqVisItDatabaseBridgePanel::~pqVisItDatabaseBridgePanel()
{
  #if defined vtkVisitDatabaseBridgeDEBUG
  cerr << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::~pqVisItDatabaseBridgePanel" << endl;
  #endif

  delete this->Form;

  this->VTKConnect->Delete();
  this->VTKConnect=0;

  if (this->DatabaseView)
    {
    this->DatabaseView->Delete();
    }

  this->Items.clear();
}

//-----------------------------------------------------------------------------
void pqVisItDatabaseBridgePanel::UpdateInformationEvent()
{
  #if defined vtkVisitDatabaseBridgeDEBUG
  cerr << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::UpdateInformationEvent" << endl;
  #endif

  vtkSMProxy* dbbProxy=this->referenceProxy()->getProxy();

  // Get the SIL view, which has been serialzed in the serverside pipeline 
  // information. See vtkVisitDtabaseBridge::RequestInformation.
  vtkProcessModule* pm=vtkProcessModule::GetProcessModule();
  vtkPVSILInformation* info=vtkPVSILInformation::New();
  pm->GatherInformation(
      dbbProxy->GetConnectionID(),
      vtkProcessModule::DATA_SERVER,
      info,
      dbbProxy->GetID());
  vtkGraph *dbView=info->GetSIL();
  if (dbView==NULL)
    {
    vtkGenericWarningMacro("Could not get the database view.  Aborting.");
    info->Delete();
    return;
    }
  // UpdateInformationEvents are fired all the time even when the
  // RequestInformation hasn't been called on the server side. Therefor
  // we need to discriminate which events we respond to. If we are out of 
  // date regenerate our view.
  int viewMTimeId=dbView->GetVertexData()->GetNumberOfArrays()-1;
  vtkIntArray *viewMTimeArray
    = dynamic_cast<vtkIntArray *>(dbView->GetVertexData()->GetAbstractArray(viewMTimeId));
  bool ok;
  int viewMTime=viewMTimeArray->GetValue(0);
  if (viewMTime>this->ViewMTime)
    {
    #if defined vtkVisitDatabaseBridgeDEBUG
    cerr << "Updating database view." << endl;
    #endif
    // Update the modified time.
    this->ViewMTime=viewMTime;
    // Save a reference to the new view.
    if (this->DatabaseView)
      {
      this->DatabaseView->Delete();
      this->DatabaseView=NULL;
      }
    this->DatabaseView=dbView;
    this->DatabaseView->Register(NULL);
    // Resize array to hold pointers to tree items,indexed by vertex.
    vtkIdType nVerts=this->DatabaseView->GetNumberOfVertices();
    this->Items.clear();
    this->Items.resize(nVerts,NULL);
    // Copy the tree into our panel's qTreeWidget.
    this->Form->DatabaseView->clear();
    QTreeWidgetItem *p=this->Form->DatabaseView->invisibleRootItem();
    vtkOutEdgeIterator *eit=vtkOutEdgeIterator::New();
    this->DatabaseView->GetOutEdges(0,eit);
    CopySIL(this->DatabaseView,eit,0,this->Form->DatabaseView,this->Items);
    eit->Delete();
    }
  //
  info->Delete();
}

//-----------------------------------------------------------------------------
void pqVisItDatabaseBridgePanel::UpdateCheckState(QTreeWidgetItem *item, int col)
{
  #if defined vtkVisitDatabaseBridgeDEBUG
  cerr << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::UpdateCheckState" << endl;
  #endif

  // More signals are fired as we change linked to/from items'
  // check state, we want to ignore these.
  static bool ignore=false;
  if (ignore)
    {
    return;
    }
  ignore=true;

  int vert=item->data(0,QT_SS_VERT).toInt();
  Qt::CheckState state=item->checkState(0);

  #if defined vtkVisitDatabaseBridgeDEBUG
  cerr << item->text(0).toStdString()
       << " vert " << vert << " state " << state << endl;
  #endif

  // Check for links out. Links out are followed and their check state
  // is syncronized with this one.
  UpdateCheckStateOut(this->DatabaseView,this->Items,vert,state);
  // Check for links in. Links in will be unchecked if this item
  // is being unchecked otherwise their state will not change.
  UpdateCheckStateIn(this->DatabaseView,this->Items,vert,state);

  ignore=false;
}

//-----------------------------------------------------------------------------
void pqVisItDatabaseBridgePanel::accept()
{
  #if defined vtkVisitDatabaseBridgeDEBUG
  cerr << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::accept" << endl;
  #endif

  vtkIntArray *ssIds
    = dynamic_cast<vtkIntArray *>(this->DatabaseView->GetVertexData()->GetAbstractArray(1));
  vtkIntArray *ssRole
    = dynamic_cast<vtkIntArray *>(this->DatabaseView->GetVertexData()->GetAbstractArray(2));

  // These data structures mirror those on the server, they will be 
  // loaded and pushed.
  vector<int> meshIds(1,0);           // The first element is expected to be the length
  vector<vector<int> > arrayIds;      // of the array.
  vector<vector<int> > expressionIds;
  vector<vector<int> > domainIds;
  vector<vector<int> > blockIds;
  vector<vector<int> > assemblyIds;
  vector<vector<int> > materialIds;
  vector<vector<int> > speciesIds;

  // The q tree widget's top level contains what ever meshes were 
  // found in the database. We need to goup our communication with 
  // the sever side mesh by mesh because we have no way of sending a 
  // tree or a vector of vectors.
  int meshIdx=-1;
  const int nMeshes=this->Form->DatabaseView->topLevelItemCount();
  for (int meshId=0; meshId<nMeshes; ++meshId)
    {
    QTreeWidgetItem *meshItem=this->Form->DatabaseView->topLevelItem(meshId);
    // If nothing is checked then this mesh won't be processed.
    if (meshItem->checkState(0)==Qt::Unchecked)
      {
      #if defined vtkVisitDatabaseBridgeDEBUG
      cerr << "Skipping mesh " << meshId << " because nothing is checked." << endl;
      #endif
      continue;
      }
    meshIds.push_back(meshId);
    ++meshIdx;
    // Set up container to hold ids of checked items under this mesh.
    arrayIds.push_back(vector<int>(2,0));      // The first two elements are expected to be the 
    expressionIds.push_back(vector<int>(2,0)); // meshId and the length of the array. This works
    domainIds.push_back(vector<int>(2,0));     // around the fact that only fixed length arrays
    blockIds.push_back(vector<int>(2,0));      // are supported by ServerManager.
    assemblyIds.push_back(vector<int>(2,0));
    materialIds.push_back(vector<int>(2,0));
    speciesIds.push_back(vector<int>(2,0));
    // Each mesh has a number of categories, each of the categories
    // has a number of checked items which we should send to the 
    // right server side property.
    const int nCategories=meshItem->childCount();
    for (int catId=0; catId<nCategories; ++catId)
      {
      QTreeWidgetItem *catItem=meshItem->child(catId);
      const int nItems=catItem->childCount();
      for (int itemId=0; itemId<nItems; ++itemId)
        {
        QTreeWidgetItem *item=catItem->child(itemId);
        if (item->checkState(0)!=Qt::Checked)
          {
          // only checked items, skip the rest.
          continue;
          }
        int vert=item->data(0,QT_SS_VERT).toInt();
        switch (ssRole->GetValue(vert))
            {
            // Data arrays.
            case SIL_USERD_ARRAY:
              arrayIds[meshIdx].push_back(ssIds->GetValue(vert));
              break;
            // Expressions.
            case SIL_USERD_EXPRESSION:
              expressionIds[meshIdx].push_back(ssIds->GetValue(vert));
              break;
            // Domain related categories. Each can be treated seperately
            // so that complex set operations can take place on the server 
            // side.
            case SIL_BLOCK:
              if (0) // TODO set ops
                {
                blockIds[meshIdx].push_back(ssIds->GetValue(vert));
                }
              break;
            case SIL_ASSEMBLY:
              if (0) // TODO set ops
                {
                assemblyIds[meshIdx].push_back(ssIds->GetValue(vert));
                }
              break;
            case SIL_DOMAIN:
              if (1) // TODO set ops
                {
                domainIds[meshIdx].push_back(ssIds->GetValue(vert));
                }
              break;
            // Material related categories. What exactly are species?
            case SIL_MATERIAL:
              if (1) // TODO set ops
                {
                materialIds[meshIdx].push_back(ssIds->GetValue(vert));
                }
              break;
            case SIL_SPECIES:
              if (1) // TODO set ops
                {
                speciesIds[meshIdx].push_back(ssIds->GetValue(vert));
                }
              break;
            // These are some categories which we are ignoring as 
            // we don't have a good feel on how they are used.
            case SIL_BOUNDARY:
            case SIL_TOPOLOGY:
            case SIL_PROCESSOR:
            case SIL_ENUMERATION:
            case SIL_USERD:
            default:
              break;
            }
        }
      }
    }
  // Now that we have collected all of the checked items into 
  // containers that fulfill server-side configuration requirements
  // we push them.
  vtkSMProxy* dbbProxy=this->referenceProxy()->getProxy();
  // MeshIds must go first as this push will also initialize the other
  // conatiners. If there are no meshes to process this ends up clearing
  // everything serverside. That's why we do it either way.
  meshIds[0]=meshIds.size(); // Size in first element is always expected.
  vtkSMIntVectorProperty *meshProp
    = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("SetMeshIds"));
  meshProp->SetNumberOfElements(meshIds.size());
  meshProp->SetElements(&meshIds[0]);
  meshProp->Modified();
  dbbProxy->UpdateProperty("SetMeshIds");
  // Push the other properites mesh by mesh.
  const int nMeshesToProcess=meshIds.size()-1;
  if (nMeshesToProcess>0)
    {
    // Get the properties outside of the meshId loop.
    vtkSMIntVectorProperty *arrayProp
      = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("SetArrayIds"));
    vtkSMIntVectorProperty *expressionProp
      = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("SetExpressionIds"));
    vtkSMIntVectorProperty *domainProp
      = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("SetDomainSSIds"));
    vtkSMIntVectorProperty *blockProp
      = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("SetBlockSSIds"));
    vtkSMIntVectorProperty *assemblyProp
      = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("SetAssemblySSIds"));
    vtkSMIntVectorProperty *materialProp
      = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("SetMaterialSSIds"));
    vtkSMIntVectorProperty *speciesProp
      = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("SetSpeciesSSIds"));
    // For each mesh we have to process, send the selected prroperties.
    for (int idx=0; idx<nMeshesToProcess; ++idx)
      {
      int n=0;
      int meshIdx=idx+1;
      // Send arrays.
      n=arrayIds[idx].size();
      if (n>2) // n<=2 means nothing selected.
        {
        arrayIds[idx][0]=n; // Size and meshId are always expected.
        arrayIds[idx][1]=idx;//meshIds[meshIdx];
        arrayProp->SetNumberOfElements(n);
        arrayProp->SetElements(&arrayIds[idx][0]);
        arrayProp->Modified();
        dbbProxy->UpdateProperty("SetArrayIds");
        #if defined vtkVisitDatabaseBridgeDEBUG
        cerr << "Selected arrays: " << arrayIds[idx] << endl;
        #endif
        }
      // Send expressions.
      n=expressionIds[idx].size();
      if (n>2)
        {
        expressionIds[idx][0]=n; // Size and meshId are always expected.
        expressionIds[idx][1]=idx;//meshIds[meshIdx];
        expressionProp->SetNumberOfElements(expressionIds[idx].size());
        expressionProp->SetElements(&expressionIds[idx][0]);
        expressionProp->Modified();
        dbbProxy->UpdateProperty("SetExpressionIds");
        #if defined vtkVisitDatabaseBridgeDEBUG
        cerr << "Selected expressions: " << expressionIds[idx] << endl;
        #endif
        }
      // Send domains.
      n=domainIds[idx].size();
      if (n>2)
        { 
        domainIds[idx][0]=n; // Size and meshId are always expected.
        domainIds[idx][1]=idx;//meshIds[meshIdx];
        domainProp->SetNumberOfElements(domainIds[idx].size());
        domainProp->SetElements(&domainIds[idx][0]);
        domainProp->Modified();
        dbbProxy->UpdateProperty("SetDomainSSIds");
        #if defined vtkVisitDatabaseBridgeDEBUG
        cerr << "Selected domains: " << domainIds[idx] << endl;
        #endif
        }
      // Send blocks.
      n=blockIds[idx].size();
      if (n>2)
        {
        blockIds[idx][0]=n; // Size and meshId are always expected.
        blockIds[idx][1]=meshIds[meshIdx];
        blockProp->SetNumberOfElements(blockIds[idx].size());
        blockProp->SetElements(&blockIds[idx][0]);
        blockProp->Modified();
        dbbProxy->UpdateProperty("SetBlockSSIds");
        }
      // Send assemblys.
      n=assemblyIds[idx].size();
      if (n>2)
        {
        assemblyIds[idx][0]=n; // Size and meshId are always expected.
        assemblyIds[idx][1]=meshIds[meshIdx];
        assemblyProp->SetNumberOfElements(assemblyIds[idx].size());
        assemblyProp->SetElements(&assemblyIds[idx][0]);
        assemblyProp->Modified();
        dbbProxy->UpdateProperty("SetAssemblySSIds");
        }
      // Send materials.
      n=materialIds[idx].size();
      if (n>2)
        {
        materialIds[idx][0]=n; // Size and meshId are always expected.
        materialIds[idx][1]=idx;//meshIds[meshIdx];
        materialProp->SetNumberOfElements(materialIds[idx].size());
        materialProp->SetElements(&materialIds[idx][0]);
        materialProp->Modified();
        dbbProxy->UpdateProperty("SetMaterialSSIds");
        #if defined vtkVisitDatabaseBridgeDEBUG
        cerr << "Selected materials: " << materialIds[idx] << endl;
        #endif
        }
      // Send speciess.
      n=speciesIds[idx].size();
      if (n>2)
        {
        speciesIds[idx][0]=n; // Size and meshId are always expected.
        speciesIds[idx][1]=meshIds[meshIdx];
        speciesProp->SetNumberOfElements(speciesIds[idx].size());
        speciesProp->SetElements(&speciesIds[idx][0]);
        speciesProp->Modified();
        dbbProxy->UpdateProperty("SetSpeciesSSIds");
        }
      }
    }
  // Catch all, update anything else that may need updating.
  dbbProxy->UpdateVTKObjects();
  // Let our superclass do the undocumented stuff that needs to be done.
  pqNamedObjectPanel::accept();
}

/// TODO SET OPS
// Set the set operation used to combine domains and blocks.
//   if (this->Form->ddcbDomBlockSetOp->isEnabled())
//     {
//     int idx=this->Form->ddcbDomBlockSetOp->currentIndex();
//     int setOp
//       =this->Form->ddcbDomBlockSetOp->itemData(idx).toInt();
// 
//     vtkSMIntVectorProperty *domainBlockProp
//       = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("SetDomainToBlockSetOperation"));
//     domainBlockProp->SetElement(0,setOp);
//     dbbProxy->UpdateProperty("SetDomainToBlockSetOperation");
//     }
//   // Set the set operation used to combine domains and assemblies.
//   if (this->Form->ddcbDomAssemSetOp->isEnabled())
//     {
//     int idx=this->Form->ddcbDomAssemSetOp->currentIndex();
//     int setOp
//       =this->Form->ddcbDomAssemSetOp->itemData(idx).toInt();
// 
//     vtkSMIntVectorProperty *domainAssemProp
//       = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("SetDomainToAssemblySetOperation"));
//     domainAssemProp->SetElement(0,setOp);
//     }
//   // Set the set operation used to combine domains and materials.
//   if (this->Form->ddcbDomMatSetOp->isEnabled())
//     {
//     int idx=this->Form->ddcbDomMatSetOp->currentIndex();
//     int setOp
//       =this->Form->ddcbDomMatSetOp->itemData(idx).toInt();
// 
//     vtkSMIntVectorProperty *domainMatProp
//       = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("SetDomainToMaterialSetOperation"));
//     domainMatProp->SetElement(0,setOp);
//     }
/// TODO SET OPS
// Set up SIL set operation drop downs.
//   this->Form->ddcbDomBlockSetOp->clear();
//   this->Form->ddcbDomBlockSetOp->addItem("Union",QVariant(SIL_OPERATION_UNION));
//   this->Form->ddcbDomBlockSetOp->addItem("Intersect",QVariant(SIL_OPERATION_INTERSECT));
//   this->Form->ddcbDomAssemSetOp->clear();
//   this->Form->ddcbDomAssemSetOp->addItem("Union",QVariant(SIL_OPERATION_UNION));
//   this->Form->ddcbDomAssemSetOp->addItem("Intersect",QVariant(SIL_OPERATION_INTERSECT));
//   this->Form->ddcbDomMatSetOp->clear();
//   this->Form->ddcbDomMatSetOp->addItem("Intersect",QVariant(SIL_OPERATION_INTERSECT));
//   this->Form->ddcbDomMatSetOp->addItem("Union",QVariant(SIL_OPERATION_UNION));
//   QObject::connect(
//       this->Form->ddcbDomBlockSetOp,
//       SIGNAL(currentIndexChanged(int)),
//       this, SLOT(setModified()));
//   QObject::connect(
//       this->Form->ddcbDomAssemSetOp,
//       SIGNAL(currentIndexChanged(int)),
//       this, SLOT(setModified()));
//   QObject::connect(
//       this->Form->ddcbDomMatSetOp,
//       SIGNAL(currentIndexChanged(int)),
//       this, SLOT(setModified()));
/// TODO  SET OPS
//   // Disable SIL set operation drop downs, if the relevant categories are
//   // present in the database they will be enabled. The drop down's first
//   // item is its default.
//   this->Form->ddcbDomBlockSetOp->setEnabled(false);
//   this->Form->ddcbDomBlockSetOp->setCurrentIndex(0);
//   this->Form->ddcbDomAssemSetOp->setEnabled(false);
//   this->Form->ddcbDomAssemSetOp->setCurrentIndex(0);
//   this->Form->ddcbDomMatSetOp->setEnabled(false);
//   this->Form->ddcbDomMatSetOp->setCurrentIndex(0);
//   // In the newly initialized tree view the top level items are
//   // meshes. The children of the mesh items are categories
//   // enable set operations only for categories that are
//   // present.
//   int nMeshes=this->Form->DatabaseView->topLevelItemCount();
//   for (int meshId=0; meshId<nMeshes; ++meshId)
//     {
//     QTreeWidgetItem  *meshItem=this->Form->DatabaseView->topLevelItem(meshId);
//     int nCategories=meshItem->childCount();
//     for (int catId=0; catId<nCategories; ++catId)
//       {
//       QTreeWidgetItem *catItem=meshItem->child(catId);
//       int role=catItem->data(0,QT_SS_ROLE).toInt();
//       switch(role)
//         {
//         case SIL_BLOCK:
//           this->Form->ddcbDomBlockSetOp->setEnabled(true);
//           break;
//         case SIL_ASSEMBLY:
//           this->Form->ddcbDomAssemSetOp->setEnabled(true);
//           break;
//         case SIL_MATERIAL:
//           this->Form->ddcbDomMatSetOp->setEnabled(true);
//           break;
//         }
//       }
//     }
/// TODO SET OPS
//   // The domain set operation mode determines how the final SIL is 
//   // constructed. NEEDS MORE DOC.
//   int dsoIdx=this->Form->ddcbDomainSetOperation->currentIndex();
//   int dsoSetOp
//     =this->Form->ddcbDomainSetOperation->itemData(dsoIdx).toInt();

// if (p && (role==SIL_DOMAIN || role==SIL_ASSEMBLY || role==SIL_BLOCK))
//   { 
//   // Set check state for categories involving domains so that
//   // by default all are active.
//   item->setCheckState(0,Qt::Checked);
//   }

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
/// How a wiget in custom panel tells PV that things need to be applied
// QObject::connect(
//     this->Form->DatabaseView,
//     SIGNAL(itemChanged(QTreeWidgetItem*, int)),
//     this, SLOT(setModified()));
