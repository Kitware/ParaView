import sys

from paraview.simple import *

#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

for i, arg in enumerate(sys.argv):
    if arg == "-D" and i+1 < len(sys.argv):
        double_mach = sys.argv[i+1] + '/Testing/Data/double_mach_reflection/plt00030.temp'
        amrex_mfix = sys.argv[i+1] + '/Testing/Data/AMReX-MFIX/plt00005'
try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print ("Could not get baseline directory. Test failed.")

# Create a new 'AMReXBoxLibGrid Reader'
amrex_mfix_ref = AMReXBoxLibGridReader(registrationName='amrex_mfix_ref', FileNames=[amrex_mfix])
amrex_mfix_ref.CellArrayStatus = ['ep_g', 'mu_g', 'p_g', 'ro_g', 'rop_g', 'u_g', 'v_g', 'volfrac', 'vort', 'w_g']

# Check the source is loaded
assert(FindSource("amrex_mfix_ref") is not None)

# Check that the loaded source is the correct one
for field in ('ep_g', 'mu_g', 'p_g', 'ro_g', 'rop_g', 'u_g', 'v_g', 'volfrac', 'vort', 'w_g'):
  assert(amrex_mfix_ref.CellData.GetArray(field) is not None)

# Change file
ReplaceReaderFileName(amrex_mfix_ref, [double_mach], 'FileNames')

# Check that previous source does not exist anymore
assert(FindSource("amrex_mfix_ref") is None)

# Check that new source is loaded
double_mach_ref = FindSource('plt00030.temp')
assert(double_mach_ref is not None)

double_mach_ref.CellArrayStatus = ['Temp', 'density', 'pressure', 'rho_E', 'rho_X', 'rho_e', 'xmom', 'ymom', 'zmom']

# Check that the new loaded source is the correct one (i.e different from the previous one)
for field in ('Temp', 'density', 'pressure', 'rho_E', 'rho_X', 'rho_e', 'xmom', 'ymom', 'zmom'):
  assert(double_mach_ref.CellData.GetArray(field) is not None)
