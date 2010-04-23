/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionStreamer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelectionStreamer -  streamer for vtkSelection
// .SECTION Description
// vtkSelectionStreamer is a streamer for vtkSelection corresponding to
// vtkTableStreamer. This is used to sections from input vtkSelection relevant
// to the sections passed through by a vtkTableStreamer with same attributes and
// inputs.

#ifndef __vtkSelectionStreamer_h
#define __vtkSelectionStreamer_h

#include "vtkTableStreamer.h"

class vtkSelection;
class vtkSelectionNode;
class vtkCompositeDataIterator;

class VTK_EXPORT vtkSelectionStreamer : public vtkTableStreamer
{
public:
  static vtkSelectionStreamer* New();
  vtkTypeMacro(vtkSelectionStreamer, vtkTableStreamer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Select the field to process. Only selection associated with the chosen
  // field are considered.
  // Accepted values are 
  // \li vtkDataObject::FIELD_ASSOCIATION_POINTS,
  // \li vtkDataObject::FIELD_ASSOCIATION_CELLS,
  // \li vtkDataObject::FIELD_ASSOCIATION_NONE,
  // \li vtkDataObject::FIELD_ASSOCIATION_VERTICES,
  // \li vtkDataObject::FIELD_ASSOCIATION_EDGES,
  // \li vtkDataObject::FIELD_ASSOCIATION_ROWS
  // If value is vtkDataObject::FIELD_ASSOCIATION_NONE, then FieldData
  // associated with the input dataobject is extracted.
  vtkSetMacro(FieldAssociation, int);
  vtkGetMacro(FieldAssociation, int);

//BTX
protected:
  vtkSelectionStreamer();
  ~vtkSelectionStreamer();

  // Description:
  // Overridden to fill information about the selection input.
  virtual int FillInputPortInformation(int port, vtkInformation* info);
  int FillOutputPortInformation(int port, vtkInformation* info);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestDataObject(vtkInformation*,
                                vtkInformationVector**,
                                vtkInformationVector*) { return 1; }

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);

  vtkSelectionNode* LocateSelection(
    vtkCompositeDataIterator* inputIter, vtkSelection* sel);

  vtkSelectionNode* LocateSelection(vtkSelection* sel);

  bool LocateSelection(vtkSelectionNode* node);

  bool PassBlock(vtkSelectionNode* output, vtkSelectionNode* input,
    vtkIdType offset, vtkIdType count);

  int FieldAssociation;
private:
  vtkSelectionStreamer(const vtkSelectionStreamer&); // Not implemented
  void operator=(const vtkSelectionStreamer&); // Not implemented
//ETX
};

#endif

