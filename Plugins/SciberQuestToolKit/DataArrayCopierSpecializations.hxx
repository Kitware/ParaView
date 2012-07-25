/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef __DataArrayCopierSpecializations_h
#define __DataArrayCopierSpecializations_h

#include "DataArrayCopierImpl.hxx"

#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkUnsignedCharArray.h"

// for convenience

typedef DataArrayCopierImpl<vtkIntArray> IntDataArrayCopier;
typedef DataArrayCopierImpl<vtkIdTypeArray> IdTypeDataArrayCopier;
typedef DataArrayCopierImpl<vtkFloatArray> FloatDataArrayCopier;
typedef DataArrayCopierImpl<vtkDoubleArray> DoubleDataArrayCopier;
typedef DataArrayCopierImpl<vtkUnsignedCharArray> UnsignedCharDataArrayCopier;

#endif
