/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "GDAMetaDataKeys.h"

#include "vtkInformationKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"

//-----------------------------------------------------------------------------
vtkInformationKeyRestrictedMacro(GDAMetaDataKeys,DIPOLE_CENTER,DoubleVector,3);

//-----------------------------------------------------------------------------
vtkInformationKeyMacro(GDAMetaDataKeys,CELL_SIZE_RE,Double);

//-----------------------------------------------------------------------------
vtkInformationKeyMacro(GDAMetaDataKeys,PULL_DIPOLE_CENTER,Integer);
