/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "vtkSQOOCReader.h"

#include "vtkInformationKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerVectorKey.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSQOOCReader, "$Revision: 0.0 $");

//-----------------------------------------------------------------------------
vtkInformationKeyMacro(vtkSQOOCReader,READER,ObjectBase);

//-----------------------------------------------------------------------------
vtkInformationKeyRestrictedMacro(vtkSQOOCReader,BOUNDS,DoubleVector,6);

//-----------------------------------------------------------------------------
vtkInformationKeyRestrictedMacro(vtkSQOOCReader,PERIODIC_BC,IntegerVector,3);

//-----------------------------------------------------------------------------
void vtkSQOOCReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent.GetNextIndent());
  os << indent << "TimeIndex: " << this->TimeIndex << endl;
  os << indent << "Time:      " << this->Time << endl;
}
