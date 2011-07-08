/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef GDAMetaDataKeys_h
#define GDAMetaDataKeys_h

class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;

class GDAMetaDataKeys
{
public:
  static vtkInformationDoubleVectorKey* DIPOLE_CENTER();
  static vtkInformationIntegerKey *PULL_DIPOLE_CENTER();
  static vtkInformationDoubleKey* CELL_SIZE_RE();
};

#endif
