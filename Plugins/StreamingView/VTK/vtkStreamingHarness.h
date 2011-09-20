/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamingHarness.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStreamingHarness - a control point for streaming
// .SECTION Description
// This is a control point to orchestrate streamed pipeline passes.
// Insert this filter at the end of a pipeline and give it a number of passes.
// It will split any downstream requests it gets into that number of sub-passes.
// It does not automate iteration over those passes. That is left to the
// application which does so by giving it a particular pass number and optional
// resolution. Controls are also included to simplify the task of asking
// for priority computations and meta information.

#ifndef __vtkStreamingHarness_h
#define __vtkStreamingHarness_h

#include "vtkPassInputTypeAlgorithm.h"

class vtkPieceList;
class vtkPieceCacheFilter;
class vtkGarbageCollector;

class VTK_EXPORT vtkStreamingHarness : public vtkPassInputTypeAlgorithm
{
public:
  static vtkStreamingHarness *New();
  vtkTypeMacro(vtkStreamingHarness, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //control over pass number.
  //if prioritization is not in effect this is the same thing as Piece
  //otherwise it is not
  //NOTE: this is a placeholder for callers to use, it is up to them to
  //do the indirection from abstract Pass number to absolute Piece number
  //since that can be done in any number of ways, this class does not attempt
  //to do so anywhere internally.
  vtkSetMacro(Pass, int);
  vtkGetMacro(Pass, int);

  //Description:
  //control over absolute piece to request
  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);

  //Description:
  //control over number of pieces that the pipeline can be split into
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);

  //Description:
  //control over resolution
  void SetResolution(double r);
  vtkGetMacro(Resolution, double);

  //Description:
  //compute the priority for a particular piece.
  //internal Piece, NumberOfPieces, Resolution are not affected or used
  double ComputePiecePriority(int Pieces, int NumPieces, double Resolution);

  //Description:
  //compute the meta information for a particular piece.
  //internal Piece, NumberOfPieces, Resolution are not affected or used
  //TODO: need a general struct for meta info with type correct bounds,
  //and room for all arrays not just active scalars
  void ComputePieceMetaInformation
    (int Piece, int NumPieces, double Resolution,
     double bounds[6], double &geometric_confidence,
     double &min, double &max, double &attribute_confidence);
  void ComputePieceMetaInformation
    (int Piece, int NumPieces, double Resolution,
     double bounds[6], double &geometric_confidence,
     double &min, double &max, double &attribute_confidence,
     unsigned long &numCells, double **pNormal);

  //Description:
  //determines if the piece is in the piece cache filter's append slot
  bool InAppend(int Pieces, int NumPieces, double Resolution);

  //Description:
  //Handle to storage for the computed meta-data and priorities.
  //This may or may not be present but when they are, having the handle
  //here simplifies the driver's code to manage the information
  void SetPieceList1(vtkPieceList *);
  vtkGetObjectMacro(PieceList1, vtkPieceList);
  void SetPieceList2(vtkPieceList *);
  vtkGetObjectMacro(PieceList2, vtkPieceList);

  //Description:
  //Handle to immediately upstream cache filter.
  //This may or may not be present, but when so having the handle here simplifies
  //the driver's code to manage the cache.
  void SetCacheFilter(vtkPieceCacheFilter *);
  vtkGetObjectMacro(CacheFilter, vtkPieceCacheFilter);

  //Description:
  //In multiresolution streaming, this prevents the associated object from
  //changing resolution level. Default is 0, off.
  void SetLockRefinement(int );
  vtkGetMacro(LockRefinement, int);

  //Description:
  //In multiresolution streaming, this causes the associated object to
  //restart at the lowest resolution.
  void RestartRefinement();

  // Description:
  //When an actor or representation containing this harness is not visible
  //the driver checks this to ignore this harness entirely.
  vtkSetMacro(Enabled, bool);
  vtkGetMacro(Enabled, bool);

  // Description:
  // Tells this to ask the upstream PCF to prepare appended results.
  void Append();

protected:
  vtkStreamingHarness();
  ~vtkStreamingHarness();

  virtual int ProcessRequest(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  virtual int RequestUpdateExtent(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  virtual int RequestData(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  int Pass;
  int Piece;
  int NumberOfPieces;
  double Resolution;
  bool ForOther;

  vtkPieceList *PieceList1;
  vtkPieceList *PieceList2;
  vtkPieceCacheFilter *CacheFilter;
  bool TryAppended;

  int LockRefinement;
  bool Enabled;

  virtual void ReportReferences(vtkGarbageCollector*);
private:
  vtkStreamingHarness(const vtkStreamingHarness&);  // Not implemented.
  void operator=(const vtkStreamingHarness&);  // Not implemented.
};


#endif
