import sys
import paraview
import paraview.vtk as vtk
import paraview.simple as pvsimple
import math
import time
import os

fEnsemble=open("wavelet.evtp", "w")
fEnsemble.write("YMag,ZMag,\n")
numSteps = 10
currentDir = os.getcwd()
for yMag in (18, 36, 72):
    for zMag in (5, 10, 20):
        fileNameTime = currentDir + "/wavelet_{:0d}_{:0d}.tvtp".format (
            yMag, zMag)
        fTime = open(fileNameTime, "w");
        fEnsemble.write ("{:d},{:d},{:s}\n".format(yMag, zMag, fileNameTime))
        for step in range(numSteps):
            print("Timestep %d" % step)

            wavelet = pvsimple.Wavelet()
            wavelet.Maximum = 300+50*math.sin(step * 2 * 3.1415927 / 10)
            wavelet.YMag = yMag
            wavelet.ZMag = zMag

            contour = pvsimple.Contour(guiName="Contour",
                                       Isosurfaces=[230.0],
                                       ContourBy=['RTData'],
                                       PointMergeMethod="Uniform Binning" )


            fileName = currentDir + "/wavelet_{:0d}_{:0d}_{:0d}.vtp".format (
                yMag, zMag, step)
            writer = pvsimple.XMLPolyDataWriter(Input = contour,
                                                FileName=fileName)
            writer.UpdatePipeline()

            pvsimple.Show()
            pvsimple.Render()
            pvsimple.Delete(wavelet)
            pvsimple.Delete(contour)
            pvsimple.Delete(writer)
            wavelet = None
            contour = None
            writer = None
            fTime.write(fileName)
            fTime.write("\n")
        fTime.close()
fEnsemble.close()
