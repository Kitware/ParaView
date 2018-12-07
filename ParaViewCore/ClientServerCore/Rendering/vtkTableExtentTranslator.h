/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableExtentTranslator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTableExtentTranslator
 * @brief   Extent translation through lookup table.
 *
 * vtkTableExtentTranslator provides a vtkExtentTranslator that is
 * programmed with a specific extent corresponding to each piece
 * number.  Readers can provide this to an application to allow the
 * pipeline to execute using the same piece breakdown that is provided
 * in the input file.
*/

#ifndef vtkTableExtentTranslator_h
#define vtkTableExtentTranslator_h

#include "vtkExtentTranslator.h"
#include "vtkPVClientServerCoreRenderingModule.h" // For export macro

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkTableExtentTranslator : public vtkExtentTranslator
{
public:
  vtkTypeMacro(vtkTableExtentTranslator, vtkExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkTableExtentTranslator* New();

  // Set the number of pieces into which the whole extent will be
  // split.  If this is 1 then the whole extent will be returned.  If
  // this is more than the number of pieces in the table then the
  // extra pieces will be empty data.  If this is more than one but
  // less than the number of pieces in the table then only this many
  // pieces will be returned (FIXME).
  void SetNumberOfPieces(int pieces) override;

  //@{
  /**
   * Set the real number of pieces in the extent table.
   */
  void SetNumberOfPiecesInTable(int pieces);
  vtkGetMacro(NumberOfPiecesInTable, int);
  //@}

  /**
   * Called to translate the current piece into an extent.  This is
   * not thread safe.
   */
  int PieceToExtent() override;

  /**
   * Not supported by this subclass of vtkExtentTranslator.
   */
  int PieceToExtentByPoints() override;

  /**
   * Not supported by this subclass of vtkExtentTranslator.
   */
  int PieceToExtentThreadSafe(int piece, int numPieces, int ghostLevel, int* wholeExtent,
    int* resultExtent, int splitMode, int byPoints) override;

  /**
   * Set the extent to be used for a piece.  This sets the extent table
   * entry for the piece.
   */
  virtual void SetExtentForPiece(int piece, int* extent);

  //@{
  /**
   * Get the extent table entry for the given piece.  This is only for
   * code that is setting up the table.  Extent translation should
   * always be done through the PieceToExtent method.
   */
  virtual void GetExtentForPiece(int piece, int* extent);
  virtual int* GetExtentForPiece(int piece);
  //@}

  //@{
  /**
   * Set the maximum ghost level that can be requested.  This can be
   * used by a reader to make sure an extent request does not go
   * outside the boundaries of the piece's file.
   */
  vtkSetMacro(MaximumGhostLevel, int);
  vtkGetMacro(MaximumGhostLevel, int);
  //@}

  //@{
  /**
   * Get/Set whether the given piece is available.  Requesting a piece
   * that is not available will produce errors in the pipeline.
   */
  virtual void SetPieceAvailable(int piece, int available);
  virtual int GetPieceAvailable(int piece);
  //@}

protected:
  vtkTableExtentTranslator();
  ~vtkTableExtentTranslator() override;

  // Store the extent table in a single array.  Every 6 values form an extent.
  int* ExtentTable;
  int NumberOfPiecesInTable;
  int MaximumGhostLevel;

  // Store a flag for the availability of each piece.
  int* PieceAvailable;

private:
  vtkTableExtentTranslator(const vtkTableExtentTranslator&) = delete;
  void operator=(const vtkTableExtentTranslator&) = delete;
};

#endif
