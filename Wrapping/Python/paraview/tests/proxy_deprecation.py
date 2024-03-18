"""
Test paraview python backward compatibility helper for deprecated proxies.
We use GhostCellsGenerator that is deprecated in 5.12 and renamed as GhostCells.
"""
import paraview
paraview.compatibility.major = 5
paraview.compatibility.minor = 12

from paraview.simple import *

def main():
    wavelet1 = Wavelet()

    # We test if the ghost cells generator can be instantiated
    generator = GhostCellsGenerator(Input=wavelet1)

    # Verify that the concrete proxy is the new one
    if type(generator).__name__ != "GhostCells":
        print("error, proxy not substituted: is of type ", type(generator).__name__)

if __name__ == "__main__":
    main()
