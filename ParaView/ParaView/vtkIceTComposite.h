/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTComposite.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIceTComposite - The Image Composition Engine for Tiles (ICE-T) component for VTK.
// .SECTION Description
// A CompositeManager object that uses the ICE-T library for compositing.
// As such, it offers the capability of rendering to tiled displays.  By
// default, it renders to a single tile located on the processor with
// rank 0.  That is, it mimics the operation of other CompositeManagers such
// as vtkTreeComposite.
// .SECTION note
// In order for the the vtkIceTComposite class to work correctly, it can
// only be used with the vtkIceTRenderer instance of the vtkRenderer class
// (which requires OpenGL).  If any other renderer is used, warnings are
// emitted and no compositing is performed.  Calling the InstallFactory
// method will ensure that vtkIceTRenderer is the default renderer.
// .SECTION note
// Due to current limitations of the ICE-T API, only an instance of
// vtkMPIController will be accepted for a vtkMultiProcessController.
// This restriction may or may not be lifted in the future based on demand.
// .SECTION see also
// vtkIceTRenderer

#ifndef __vtkIceTComposite_h
#define __vtkIceTComposite_h

#include "vtkCompositeManager.h"

class VTK_EXPORT vtkIceTComposite : public vtkCompositeManager
{
public:
  static vtkIceTComposite *New();
  vtkTypeRevisionMacro(vtkIceTComposite, vtkCompositeManager);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Installs vtksnlParallelFactory.  Helps ensure that the appropriate
  // vtkRenderer is used with this composite manager.
  static void InstallFactory();

  virtual void SetRenderWindow(vtkRenderWindow *renWin);

  virtual void SetController(vtkMultiProcessController *controller);

  // Description:
  // Methods to set the characteristics of the tiled display.  Currently,
  // only an even grid is supported.  Numbering of tiles is 0 based.  Tiles
  // in the X direction (horizontal) are numbered from left to right.  Tiles
  // in the Y direction (vertical) are numbered from top to bottom.
  vtkGetMacro(NumTilesX, int);
  vtkGetMacro(NumTilesY, int);
  virtual void SetNumTilesX(int tilesX);
  virtual void SetNumTilesY(int tilesY);

  virtual int GetTileRank(int x, int y);
  virtual void SetTileRank(int x, int y, int rank);

//BTX
  enum StrategyType {
    DEFAULT, REDUCE, VTREE, SPLIT, SERIAL, DIRECT
  };
//ETX

  vtkGetMacro(Strategy, StrategyType);
  virtual void SetStrategy(StrategyType strategy);
  virtual void SetStrategy(const char *strategy);

  virtual double GetGetBuffersTime();
  virtual double GetSetBuffersTime();
  virtual double GetCompositeTime();
  virtual double GetMaxRenderTime();

  virtual int CheckForAbortComposite();

  // Description:
  // Basically an internal callback.
  virtual void StartRender();
  virtual void SatelliteStartRender();
  virtual void SetReductionFactorRMI();

  virtual void InitializeRMIs();

//BTX

  enum {
    SET_REDUCTION_FACTOR_RMI_TAG=622434,
    REDUCTION_FACTOR_TAG=234550,
    TILES_DIRTY_TAG=234551,
    NUM_TILES_X_TAG=234552,
    NUM_TILES_Y_TAG=234553,
    TILE_RANKS_TAG=234554,
    STRATEGY_TAG=234555
  };

//ETX

protected:
  vtkIceTComposite();
  virtual ~vtkIceTComposite();

  virtual void UpdateIceTContext(int screenWidth, int screenHeight);

  // Description:
  // We don't really compose the buffer here, but we need to override this
  // to make the compiler happy.
  virtual void CompositeBuffer(int width, int height, int useCharFlag,
             vtkDataArray *pBuf, vtkFloatArray *zBuf,
             vtkDataArray *pTmp, vtkFloatArray *zTmp);

  virtual void ChangeTileDims(int tilesX, int tilesY);

  virtual void PrepareForCompositeRender();

  IceTContext Context;
  int ContextDirty;

  int NumTilesX;
  int NumTilesY;
  int **TileRanks;
  int TilesDirty;
  int CleanScreenWidth;
  int CleanScreenHeight;

  // This is only here because SetReductionFactor is not virtual.
  int lastKnownReductionFactor;

  StrategyType Strategy;
  int StrategyDirty;

private:
  vtkIceTComposite(const vtkIceTComposite&); // Not implemented
  void operator=(const vtkIceTComposite&); // Not implemented
};


#endif //__vtkIceTComposite_h
