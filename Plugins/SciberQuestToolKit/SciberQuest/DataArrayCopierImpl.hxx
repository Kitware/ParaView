/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/

#ifndef __DataArrayCopierImpl_h
#define __DataArrayCopierImpl_h

#include "DataArrayCopier.h"
#include "DataArrayTraits.hxx"
#include "SQMacros.h"
#include "vtkDataArray.h"

/// Templated inplementation of vtkDataArray copier.
/**

*/
template<typename T>
class DataArrayCopierImpl : public DataArrayCopier
{
public:
  DataArrayCopierImpl()
      :
    NComps(0),
    Input(0),
    Output(0)
    {}

  virtual ~DataArrayCopierImpl();

  /**
  Initialize the copier for transfers from the given array
  to a new array of the same type and name. Initializes
  both input and output arrays.
  */
  virtual void Initialize(vtkDataArray *in);

  /**
  Set the array to copy from.
  */
  virtual void SetInput(vtkDataArray *in);
  virtual vtkDataArray *GetInput();

  /**
  Set the array to copy to.
  */
  virtual void SetOutput(vtkDataArray *out);
  virtual vtkDataArray *GetOutput();

  /**
  Copy the range from the input array to the end of then
  output array.
  */
  virtual void Copy(IdBlock &ids);
  virtual void Copy(vtkIdType id);

private:
  int NComps;
  T *Input;
  T *Output;

private:
  DataArrayCopierImpl(const DataArrayCopierImpl<T> &);
  void operator=(const DataArrayCopierImpl<T> &);
};

//-----------------------------------------------------------------------------
template<typename T>
DataArrayCopierImpl<T>::~DataArrayCopierImpl()
{
  this->SetInput(0);
  this->SetOutput(0);
}

//-----------------------------------------------------------------------------
template<typename T>
void DataArrayCopierImpl<T>::Initialize(vtkDataArray *in)
{
  if (in==0)
    {
    this->SetInput(0);
    this->SetOutput(0);
    sqErrorMacro(std::cerr,"Can't initialize from null pointer.");
    return;
    }

  this->SetInput(in);

  vtkDataArray *out=in->NewInstance();
  out->SetNumberOfComponents(this->NComps);
  out->SetName(in->GetName());
  this->SetOutput(out);
  out->Delete();
}

//-----------------------------------------------------------------------------
template<typename T>
void DataArrayCopierImpl<T>::SetInput(vtkDataArray *in)
{
  if (in==this->Input)
    {
    return;
    }

  if (this->Input)
    {
    this->Input->Delete();
    }

  this->Input=dynamic_cast<T*>(in);
  this->NComps=0;

  if (this->Input)
    {
    this->Input->Register(0);
    this->NComps=this->Input->GetNumberOfComponents();
    }
}

//-----------------------------------------------------------------------------
template<typename T>
vtkDataArray *DataArrayCopierImpl<T>::GetInput()
{
  return this->Input;
}

//-----------------------------------------------------------------------------
template<typename T>
void DataArrayCopierImpl<T>::SetOutput(vtkDataArray *out)
{
  if (out==this->Output)
    {
    return;
    }

  if (this->Output)
    {
    this->Output->Delete();
    }

  this->Output=dynamic_cast<T*>(out);

  if (this->Output)
    {
    this->Output->Register(0);
    }
}

//-----------------------------------------------------------------------------
template<typename T>
vtkDataArray *DataArrayCopierImpl<T>::GetOutput()
{
  return this->Output;
}

//-----------------------------------------------------------------------------
template<typename T>
void DataArrayCopierImpl<T>::Copy(vtkIdType id)
{
  // expecting scalars, vectors, and tensors
  typename DataArrayTraits<T>::InternalType val[9];

  this->Input->GetTupleValue(id,val);
  this->Output->InsertNextTupleValue(val);
}

//-----------------------------------------------------------------------------
template<typename T>
void DataArrayCopierImpl<T>::Copy(IdBlock &block)
{
  vtkIdType inAt=this->NComps*block.first();

  typename DataArrayTraits<T>::InternalType *pIn=this->Input->GetPointer(inAt);

  vtkIdType outAt=this->NComps*this->Output->GetNumberOfTuples();
  vtkIdType outN=this->NComps*block.size();

  typename DataArrayTraits<T>::InternalType *pOut=this->Output->WritePointer(outAt,outN);

  for (int i=0; i<outN; ++i,++pOut,++pIn)
    {
    *pOut=*pIn;
    }
}

#endif
