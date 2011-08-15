/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPiece - A handle for meta information about one streamed piece
// .SECTION Description
// Streaming works by driving the pipeline repeatedly over different
// subsections. This class is the way to define a subsection (P/NP@R)
// It is also a place to store meta-information gathered from the pipeline
// about that piece.

#ifndef __vtkPiece_h
#define __vtkPiece_h

class vtkPiece
{
public:
  vtkPiece();
  ~vtkPiece();

  //Description:
  // Pieces are identified generically by a fraction (P/NP) and a Resolution
  void SetPiece(int nv) { this->Piece = nv; }
  int GetPiece() { return this->Piece; }
  void SetNumPieces(int nv) { this->NumPieces = nv; }
  int GetNumPieces() { return this->NumPieces; }
  void SetResolution( double nv) { this->Resolution = nv; }
  double GetResolution() { return this->Resolution; }
  void SetProcessor(int nv) { this->Processor = nv; }
  int GetProcessor() { return this->Processor; }
  void SetReachedLimit(bool nv) { this->ReachedLimit = nv; }
  bool GetReachedLimit() { return this->ReachedLimit; }

  //Description:
  // When available, a piece's geometric bounds determines if view priority.
  void SetBounds(double *nv) {
    this->Bounds[0] = nv[0];
    this->Bounds[1] = nv[1];
    this->Bounds[2] = nv[2];
    this->Bounds[3] = nv[3];
    this->Bounds[4] = nv[4];
    this->Bounds[5] = nv[5];
  }
  double *GetBounds() { return this->Bounds; }
  void GetBounds(double *out) {
    out[0] = this->Bounds[0];
    out[1] = this->Bounds[1];
    out[2] = this->Bounds[2];
    out[3] = this->Bounds[3];
    out[4] = this->Bounds[4];
    out[5] = this->Bounds[5];
  }

  //Description:
  // The pipeline priority reflects how important the piece is to data processing
  void SetPipelinePriority(double nv) { this->PipelinePriority = nv; }
  double GetPipelinePriority() { return this->PipelinePriority; }
  //Description:
  // The view priority reflects how important the piece is based solely on its
  // position relative to the viewer
  void SetViewPriority(double nv) { this->ViewPriority = nv; }
  double GetViewPriority() { return this->ViewPriority; }
  //Description:
  // Pieces that are cached are aggregated together and then skipped individually
  void SetCachedPriority(double nv) { this->CachedPriority = nv; }
  double GetCachedPriority() { return this->CachedPriority; }
  //Description:
  // Overall priority, just Pp*PV*Pc
  double GetPriority() {
    return this->PipelinePriority * this->ViewPriority * this->CachedPriority;
  }

  //Decription:
  //Copies everything
  void CopyPiece(vtkPiece other);

  //Description:
  //Impose an ordering to for sorting
  bool ComparePriority(vtkPiece other)
  {
    return this->GetPriority() > other.GetPriority();
  }

  //Description:
  //Another ordering for comparison
  bool CompareHandle(vtkPiece other)
  {
    if (Processor<other.Processor)
      {
      return true;
      }
    if (Processor>other.Processor)
      {
      return false;
      }
    if (NumPieces < other.NumPieces)
      {
      return true;
      }
    if (NumPieces > other.NumPieces)
      {
      return false;
      }
    if (Piece < other.Piece)
      {
      return true;
      }
    return false;
  }

  bool IsValid()
  {
    return Piece!=-1;
  }

protected:

  int Processor;
  int Piece;
  int NumPieces;
  bool ReachedLimit;
  double Resolution;

  double Bounds[6];

  double PipelinePriority;
  double ViewPriority;
  double CachedPriority;

  friend class vtkPieceList;

private:
};

#endif
