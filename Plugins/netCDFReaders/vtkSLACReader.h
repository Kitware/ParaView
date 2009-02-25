// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSLACReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkSLACReader
//
// .SECTION Description
//
// A reader for a data format used by Omega3p, Tau3p, and several other tools
// used at the Standford Linear Accelerator Center (SLAC).  The underlying
// format uses netCDF to store arrays, but also imposes several conventions
// to form an unstructured grid of elements.
//

#ifndef __vtkSLACReader_h
#define __vtkSLACReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"

#include "vtkSmartPointer.h"    // For ivars
#include <vtkstd/map>           // For internal map

class vtkDataArraySelection;
class vtkIdTypeArray;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;

class vtkSLACReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkSLACReader, vtkMultiBlockDataSetAlgorithm);
  static vtkSLACReader *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  vtkGetStringMacro(MeshFileName);
  vtkSetStringMacro(MeshFileName);

  vtkGetStringMacro(ModeFileName);
  vtkSetStringMacro(ModeFileName);

  // Description:
  // If on, reads the internal volume of the data set.  Set to off by default.
  vtkGetMacro(ReadInternalVolume, int);
  vtkSetMacro(ReadInternalVolume, int);
  vtkBooleanMacro(ReadInternalVolume, int);

  // Description:
  // If on, reads the external surfaces of the data set.  Set to on by default.
  vtkGetMacro(ReadExternalSurface, int);
  vtkSetMacro(ReadExternalSurface, int);
  vtkBooleanMacro(ReadExternalSurface, int);

  // Description:
  // If on, reads midpoint information for external surfaces and builds
  // quadratic surface triangles.  Set to on by default.
  vtkGetMacro(ReadMidpoints, int);
  vtkSetMacro(ReadMidpoints, int);
  vtkBooleanMacro(ReadMidpoints, int);

  // Description:
  // Variable array selection.
  virtual int GetNumberOfVariableArrays();
  virtual const char *GetVariableArrayName(int idx);
  virtual int GetVariableArrayStatus(const char *name);
  virtual void SetVariableArrayStatus(const char *name, int status);

  // Description:
  // Returns true if the given file can be read by this reader.
  static int CanReadFile(const char *filename);

  // Description:
  // This key is attached to the metadata information of all data sets in the
  // output that are part of the internal volume.
  static vtkInformationIntegerKey *IS_INTERNAL_VOLUME();

  // Description:
  // This key is attached to the metadata information of all data sets in the
  // output that are part of the external surface.
  static vtkInformationIntegerKey *IS_EXTERNAL_SURFACE();

  // Description:
  // All the data sets stored in the multiblock output share the same point
  // data.  For convienience, the point coordinates (vtkPoints) and point data
  // (vtkPointData) are saved under these keys in the vtkInformation of the
  // output data set.
  static vtkInformationObjectBaseKey *POINTS();
  static vtkInformationObjectBaseKey *POINT_DATA();

protected:
  vtkSLACReader();
  ~vtkSLACReader();

  char *MeshFileName;
  char *ModeFileName;

  int ReadInternalVolume;
  int ReadExternalSurface;
  int ReadMidpoints;

//BTX
  vtkSmartPointer<vtkDataArraySelection> VariableArraySelection;
//ETX

  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  // Description:
  // Callback registered with the VariableArraySelection.
  static void SelectionModifiedCallback(vtkObject *caller, unsigned long eid,
                                        void *clientdata, void *calldata);

  // Description:
  // Convenience function that checks the dimensions of a 2D netCDF array that
  // is supposed to be a set of tuples.  It makes sure that the number of
  // dimensions is expected and that the number of components in each tuple
  // agree with what is expected.  It then returns the number of tuples.  An
  // error is emitted and 0 is returned if the checks fail.
  virtual vtkIdType GetNumTuplesInVariable(int ncFD, int varId,
                                           int expectedNumComponents);

  // Description:
  // True if reading from a proper mode file.  Set in RequestInformation.
  bool ReadModeData;

  // Description:
  // Read the connectivity information from the mesh file.  Returns 1 on
  // success, 0 on failure.
  virtual int ReadConnectivity(int meshFD, vtkMultiBlockDataSet *output);

  // Description:
  // Reads tetrahedron connectivity arrays.  Called by ReadConnectivity.
  virtual int ReadTetrahedronInteriorArray(int meshFD,
                                           vtkIdTypeArray *connectivity);
  virtual int ReadTetrahedronExteriorArray(int meshFD,
                                           vtkIdTypeArray *connectivity);

//BTX
  // Description:
  // Reads point data arrays.  Called by ReadCoordinates and ReadFieldData.
  virtual vtkSmartPointer<vtkDataArray> ReadPointDataArray(int ncFD, int varId);
//ETX

//BTX
  // Description:
  // Helpful constants equal to the amount of identifiers per tet.
  enum {
    NumPerTetInt = 5,
    NumPerTetExt = 9
  };
//ETX

  // Description:
  // Read in the point coordinate data from the mesh file.  Returns 1 on
  // success, 0 on failure.
  virtual int ReadCoordinates(int meshFD, vtkMultiBlockDataSet *output);

  // Description:
  // Read in the field data from the mode file.  Returns 1 on success, 0
  // on failure.
  virtual int ReadFieldData(int modeFD, vtkMultiBlockDataSet *output);

  // Description:
  // Read in the midpoint data from the mesh file.  Returns 1 on success,
  // 0 on failure.
  virtual int ReadMidpointData(int meshFD, vtkMultiBlockDataSet *output);

//BTX
  // Description:
  // Simple structure for holding midpoint information.
  class vtkMidpoint {
  public:
    vtkMidpoint() {}
    vtkMidpoint(const double coord[3], vtkIdType id) {
      this->Coordinate[0] = coord[0];
      this->Coordinate[1] = coord[1];
      this->Coordinate[2] = coord[2];
      this->ID = id;
    }
    double Coordinate[3];
    vtkIdType ID;
  };

  // Description:
  // A map from two edge midpoints to their midpoint.  This is how midpoints are
  // stored in the mesh files.
  typedef vtkstd::map<vtkstd::pair<vtkIdType, vtkIdType>, vtkMidpoint>
    vtkMidpointCoordinateMap;
//ETX

  // Description:
  // Reads in the midpoint coordinate data from the mesh file and returns a map
  // from edges to midpoints.  This method is called by ReadMidpointData.
  // Returns 1 on success, 0 on failure.
  virtual int ReadMidpointCoordinates(int meshFD, vtkMultiBlockDataSet *output,
                                      vtkMidpointCoordinateMap &map);

private:
  vtkSLACReader(const vtkSLACReader &);         // Not implemented
  void operator=(const vtkSLACReader &);        // Not implemented
};

#endif //__vtkSLACReader_h
