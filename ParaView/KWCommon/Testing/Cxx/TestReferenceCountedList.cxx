#include "vtkVector.txx"
#include "vtkLinkedList.txx"
#include "vtkActor.h"

#define C_ERROR(c) cout << "Container: " << c->GetClassName() << " "

template<class DType>
int TestList(DType* tlist)
{
  int count = 300;
  int cc;
  for ( cc = 0; cc < count; cc ++ )
    {
    vtkActor *act1 = vtkActor::New();
    //cout << "Append: " << act1 << endl;
    tlist->AppendItem(act1);
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < count; cc ++ )
    {
    vtkActor *act1 = vtkActor::New();
    //cout << "Prepend: " << act1 << endl;
    tlist->PrependItem(act1);
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < count; cc ++ )
    {
    vtkActor *act1 = vtkActor::New();
    //cout << "Insert: " << act1 << endl;
    tlist->InsertItem(cc, act1);
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < count; cc ++ )
    {
    //cout << "Remove item" << endl;
    tlist->RemoveItem(cc);
    }
  //tlist->DebugList();

  //cout << "Remove: " << tlist->GetNumberOfItems() << " items" << endl;
  tlist->RemoveAllItems();
   //tlist->DebugList();
  for ( cc = 0; cc < count; cc ++ )
    {
    vtkActor *act1 = vtkActor::New();
    //cout << "Prepend: " << act1 << endl;
    tlist->PrependItem(act1);
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < count; cc ++ )
    {
    vtkActor *act1 = vtkActor::New();
    //cout << "Insert: " << act1 << endl;
    tlist->InsertItem(cc, act1);
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < count; cc ++ )
    {
    //cout << "Remove item" << endl;
    tlist->RemoveItem(cc);
    } 
  //cout << "Remove: " << tlist->GetNumberOfItems() << " items" << endl;

  return 0;
}

int main()
{
  int res = 0;
  
  //cout << "Vector: " << endl;

  vtkVector<vtkActor*> *vv 
    = vtkVector<vtkActor*>::New();
  res += TestList(vv);
  vv->Delete();

  //cout << "Linked List: " << endl;
  vtkLinkedList<vtkActor*> *vl 
    = vtkLinkedList<vtkActor*>::New();
  res += TestList(vl);
  vl->Delete();

  return res;
}
