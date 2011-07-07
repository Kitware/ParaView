/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef __DataArrayTraits_h
#define __DataArrayTraits_h

class vtkFloatArray;
class vtkDoubleArray;
class vtkIntArray;
class vtkIdTypeArray;
class vtkUnsignedCharArray;

/// Template traits of vtkDataArrays.
/**
Provides:
  InternalType -- internal data type
*/
template <typename T>
class DataArrayTraits
{
};

/// specialization for vtkDoubleArray
template <>
class DataArrayTraits<vtkDoubleArray>
{
public:
  typedef double InternalType;
};

/// specialization for vtkFloatArray
template <>
class DataArrayTraits<vtkFloatArray>
{
public:
  typedef float InternalType;
};

/// specialization for vtkIntArray
template <>
class DataArrayTraits<vtkIntArray>
{
public:
  typedef int InternalType;
};

/// specialization for vtkIdTypeArray
template <>
class DataArrayTraits<vtkIdTypeArray>
{
public:
  typedef vtkIdType InternalType;
};

/// specialization for vtkUnsignedCharArray
template <>
class DataArrayTraits<vtkUnsignedCharArray>
{
public:
  typedef unsigned char InternalType;
};

#endif
