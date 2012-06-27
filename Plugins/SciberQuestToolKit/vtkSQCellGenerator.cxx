/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQCellGenerator.h"

#include "vtkInformationKey.h"
#include "vtkInformationObjectBaseKey.h"

//-----------------------------------------------------------------------------
vtkInformationKeyMacro(vtkSQCellGenerator,CELL_GENERATOR,ObjectBase);

//-----------------------------------------------------------------------------
void vtkSQCellGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent.GetNextIndent());
}
