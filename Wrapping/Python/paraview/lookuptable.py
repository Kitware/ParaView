r"""Utility module for easy manipultions of lookup tables.
This module is intended for use with by simple.py.


DEPRECATED: will be removed in future releases of ParaView.
"""
#==============================================================================
#
#  Program:   ParaView
#  Module:    lookuptable.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================
from __future__ import absolute_import
import os
from math import sqrt

from . import servermanager
from ._colorMaps import getColorMaps

# -----------------------------------------------------------------------------

class _vtkPVLUTData:
  """
  Internal container for ParaView lookup table data.
  Don't use this directly. Use vtkPVLUTReader.
  """
  def __init__(self):
    self.Name=""
    self.Space=""
    self.Values=[]
    self.Coords=[]

  def SetName(self,aName):
    self.Name=aName

  def GetName(self):
    return self.Name

  def SetColorSpace(self,aSpace):
    self.Space=aSpace

  def GetColorSpace(self):
    return self.Space

  def SetRGBValues(self,aValues):
    self.Values=aValues

  def GetRGBValues(self):
    return self.Values

  def SetMapCoordinates(self,aCoords):
    self.Coords=aCoords
    # normalize the coordinates
    # in preparation to map onto
    # an arbitrary scalar range
    nCoords = len(self.Coords)
    minCoord = float(min(self.Coords))
    maxCoord = float(max(self.Coords))
    deltaCoord = maxCoord - minCoord
    if (minCoord>=maxCoord):
      print ('ERROR: in coordinate values')
      return
    i=0
    while i<nCoords:
      self.Coords[i] -= minCoord
      self.Coords[i] /= deltaCoord
      i+=1
    return

  def GetMapCoordinates(self):
    return self.Coords;

  def PrintSelf(self):
    print (self.Name)
    print (self.Space)
    print (self.Values)
    print (self.Coords)


# -----------------------------------------------------------------------------

class vtkPVLUTReader:
  """
  Reader and container for ParaView's XML based lookup tables.
  Once lookup tables are loaded you access them by name. When
  accessing you must provide the array instance, which you may
  get from a pvpython 'Source' type object.

  This reader makes use of ParaView's XML LUT file format with
  one exception - the XML document must be root'ed by an element
  named "ColorMaps". Within the "ColorMaps" element an arbitrary
  number of ParaView's "ColorMap" elements define LUT entries.

  Usage:

  ::

     # at the top of your script
     # create the reader and load LUT's
     lr = lookuptable.vtkPVLUTReader()
     lr.Read('/path/to/luts.xml')
     lr.Print()

     # after you have a pvpython source object, get
     # one of it's arrays.
     srcObj = GetActiveSource()
     array = srcObj.PointData.GetArray('arrayName')

     # create a LUT for the array.
     lut = lr.GetLUT(array,'lutName')

     # set the active array and assign the LUT
     srcObjRep = Show(srcObj)
     srcObjRep.ColorArrayName = 'arrayName'
     srcObjRep.LookupTable = lut

     # finally render to display the result
     Render()

  File Format:

  ::

      <ColorMaps>
          ...
        <ColorMap name="LUTName" space="Lab,RGB,HSV" indexedLookup="true,false">
          <Point x="val" o="val" r="val" g="val" b="val"/>
            ...
          <Point x="val" o="val" r="val" g="val" b="val"/>
          <NaN r="val" g="val" b="val"/>
        </ColorMap>
          ...
        <ColorMap>
          ...
        </ColorMap>
          ...
      </ColorMaps>
  """

  def __init__(self,ns=None):
    self.LUTS={}
    self.DefaultLUT=None
    self.Globals=ns

    defaultColorMaps = getColorMaps()
    if defaultColorMaps:
      self._Read(defaultColorMaps)
    else:
      print('WARNING: default LUTs not found.')
    return

  def Clear(self):
    """
    Clear internal data structures.
    """
    self.LUTS={}
    self.DefaultLUT=None
    return

  def Read(self, aFileName):
    """
    Read in the LUT's defined in the named file. Each
    call to read extends the internal list of LUTs.
    """
    parser=servermanager.vtkPVXMLParser()
    parser.SetFileName(aFileName)
    if (not parser.Parse()):
      print ('ERROR: parsing lut file %s'%(aFileName))
      return
    root=parser.GetRootElement()
    if root.GetName()!='ColorMaps':
      print ('ERROR: parsing LUT file %s'%(aFileName))
      print ('ERROR: root element must be <ColorMaps>')
      return
    return self._Read(root)

  def _Read(self, root):
    nElems=root.GetNumberOfNestedElements()
    i=0
    nFound=0
    while (i<nElems):
      cmapElem=root.GetNestedElement(i)
      if (cmapElem.GetName()=='ColorMap'):
        nFound+=1

        lut=_vtkPVLUTData()
        lut.SetName(cmapElem.GetAttribute('name'))
        lut.SetColorSpace(cmapElem.GetAttribute('space'))

        coords=[]
        values=[]
        nRGB=cmapElem.GetNumberOfNestedElements()
        j=0
        while (j<nRGB):
          rgbElem=cmapElem.GetNestedElement(j)
          if (rgbElem.GetName()=='Point'):
            coord=float(rgbElem.GetAttribute('x'))
            coords.append(coord)
            val=[float(rgbElem.GetAttribute('r')),
              float(rgbElem.GetAttribute('g')),
              float(rgbElem.GetAttribute('b'))]
            values.append(val)
          j=j+1

        lut.SetMapCoordinates(coords)
        lut.SetRGBValues(values)
        #lut.PrintSelf()

        self.LUTS[lut.GetName()]=lut
      i=i+1

    if nFound==0:
      print ('ERROR: No ColorMaps were found in %s'%(aFileName))
    else:
      if self.DefaultLUT is None:
        names=list(self.LUTS)
        if len(names)>0:
          self.DefaultLUT=names[0]

    return nFound

  def GetLUT(self,aArray,aLutName,aRangeOveride=[]):
    """
    Given an array and lookup table name assign the LUT
    to the given array and return the LUT. If aRangeOveride
    is specified then LUT will be mapped through that
    range rather than the array's actual range.
    """
    try:
      self.LUTS[aLutName]
    except KeyError:
      if self.DefaultLUT is not None:
        print ('ERROR: No LUT named %s using %s'%(aLutName,self.DefaultLUT))
        aLutName = self.DefaultLUT
      else:
        print ('ERROR: No LUT named %s and no default available.'%(aLutName))
        return None
    range = self.__GetRange(aArray,aRangeOveride)
    return self.__GetLookupTableForArray(aArray,
               RGBPoints=self.__MapRGB(aLutName,range),
               ColorSpace=self.__GetColorSpace(aLutName),
               VectorMode='Magnitude',
               ScalarRangeInitialized=1.0)

  def GetLUTNames(self):
    """
    Return a list of the currently available LUT's names.
    """
    return sorted(iter(self.LUTS),cmp=lambda x,y: cmp(x.lower(), y.lower()))

  def Print(self):
    """
    Print the available list of LUT's.
    """
    names=""
    i=0
    for k in sorted(iter(self.LUTS),cmp=lambda x,y: cmp(x.lower(), y.lower())):
      lut=self.LUTS[k]
      names+=lut.GetName()
      names+=", "
      if ((i%6)==5):
        names+="\n"
      i+=1
    print (names)
    return
  # end of public interface

  def __GetColorSpace(self,aName):
    """
    Return the color space from the lookup table object.
    """
    return self.LUTS[aName].GetColorSpace()


  def __GetRGB(self,aName):
    """
    Return the rgb values for the named lut
    """
    return self.LUTS[aName]

  def __MapRGB(self,aName,aRange):
    """
    Map the rgb values onto a scalar range
    results are an array of [x r g b] values
    """
    colors=self.LUTS[aName].GetRGBValues()
    mapCoords=self.LUTS[aName].GetMapCoordinates()
    nColors=len(colors)
    coord0=float(aRange[0])
    coordDelta=float(aRange[1])-float(aRange[0])
    mappedColors=[]
    i=0
    while(i<nColors):
      x=coord0+coordDelta*mapCoords[i]
      val=[x]+colors[i]
      mappedColors+=val
      i=i+1
    return mappedColors

  def __GetRange(self,aArray,aRangeOveride):
    """
    Get the range from an array proxy object or if
    an override is provided use that.
    """
    nComps = aArray.GetNumberOfComponents()
    range = [0.0, 1.0]
    if (len(aRangeOveride) == 0):
      if (nComps == 1):
        range = aArray.GetRange()
      else:
        # TODO - this could be larger than the range of the magnitude aArray
        rx = aArray.GetRange(0)
        ry = aArray.GetRange(1)
        rz = aArray.GetRange(2)
        range = [0.0,
             sqrt(rx[1]*rx[1]+ry[1]*ry[1]+rz[1]*rz[1])]
    else:
      range = aRangeOveride
    return range

  def __GetLookupTableForArray(self,aArray,**kwargs):
    """
    Set the lookup table for the given array and assign
    the named properties.
    """
    proxyName='%d.%s.PVLookupTable'%(aArray.GetNumberOfComponents(),aArray.GetName())
    lut = servermanager.ProxyManager().GetProxy('lookup_tables',proxyName)
    if not lut:
      lut = servermanager.rendering.PVLookupTable(ColorSpace="HSV",RGBPoints=[0,0,0,1, 1,1,0,0])
      servermanager.Register(lut, registrationName=proxyName)
    for arg in kwargs.keys():
      if not hasattr(lut, arg):
        raise AttributeError("LUT has no property %s"%(arg))
      setattr(lut,arg,kwargs[arg])
    return lut
