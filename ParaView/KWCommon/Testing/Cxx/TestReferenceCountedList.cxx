#include "vtkVector.txx"
#include "vtkLinkedList.txx"
#include "vtkActor.h"

#define C_ERROR(c) cout << "Container: " << c->GetClassName() << " "

template<class DType>
int TestList(DType* tlist)
{
  int cc;
  for ( cc = 0; cc < 20; cc ++ )
    {
    vtkActor *act1 = vtkActor::New();
    tlist->AppendItem(act1);
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < 20; cc ++ )
    {
    vtkActor *act1 = vtkActor::New();
    tlist->PrependItem(act1);
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < 20; cc ++ )
    {
    vtkActor *act1 = vtkActor::New();
    tlist->InsertItem(cc, act1);
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < 20; cc ++ )
    {
    vtkActor *act1 = vtkActor::New();
    tlist->RemoveItem(cc);
    act1->Delete();
    }
  //tlist->DebugList();

  tlist->RemoveAllItems();
   //tlist->DebugList();
  for ( cc = 0; cc < 20; cc ++ )
    {
    vtkActor *act1 = vtkActor::New();
    tlist->PrependItem(act1);
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < 20; cc ++ )
    {
    vtkActor *act1 = vtkActor::New();
    tlist->InsertItem(cc, act1);
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < 20; cc ++ )
    {
    vtkActor *act1 = vtkActor::New();
    tlist->RemoveItem(cc);
    act1->Delete();
    } 

  return 0;
}

int main()
{
  int res = 0;
  
  //cout << "Vector: " << endl;

  vtkVector<vtkActor*> *vv 
    = vtkVector<vtkActor*>::New();
  vtkAbstractListDataIsReferenceCounted(vv);
  res += TestList(vv);
  vv->Delete();

  //cout << "Linked List: " << endl;
  vtkLinkedList<vtkActor*> *vl 
    = vtkLinkedList<vtkActor*>::New();
  vtkAbstractListDataIsReferenceCounted(vv);
  res += TestList(vl);
  vl->Delete();

  return res;
}
