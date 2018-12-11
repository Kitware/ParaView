"""This demo program solves the incompressible Navier-Stokes equations
on an L-shaped domain using Chorin's splitting method."""

# Copyright (C) 2010-2011 Anders Logg
#
# This file is part of DOLFIN.
#
# DOLFIN is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# DOLFIN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with DOLFIN. If not, see <http://www.gnu.org/licenses/>.
#
# Modified by Mikael Mortensen 2011
#
# First added:  2010-08-30
# Last changed: 2011-06-30


#
# SC14 Paraview's Catalyst tutorial
#
# Step 2 : plug to catalyst python API, add a coProcess function
#

# [SC14-Catalyst] we need a python environment that enables import of both Dolfin and ParaView
execfile("simulation-env.py")

# [SC14-Catalyst] import paraview, vtk and paraview's simple API
import sys
import paraview
import paraview.vtk as vtk
import paraview.simple as pvsimple

# [SC14-Catalyst] check for command line arguments
if len(sys.argv) != 3:
    print "command is 'python",sys.argv[0],"<script name> <number of time steps>'"
    sys.exit(1)

# [SC14-Catalyst] initialize and read input parameters
paraview.options.batch = True
paraview.options.symmetric = True

# [SC14-Catalyst] import user co-processing script
from paraview.modules import vtkPVCatalyst
import os
scriptpath, scriptname = os.path.split(sys.argv[1])
sys.path.append(scriptpath)
if scriptname.endswith(".py"):
    print 'script name is ', scriptname
    scriptname = scriptname[0:len(scriptname)-3]
try:
    cpscript = __import__(scriptname)
except:
    print sys.exc_info()
    print 'Cannot find ', scriptname, ' -- no coprocessing will be performed.'
    sys.exit(1)

# [SC14-Catalyst] Co-Processing routine to be called at the end of each simulation time step
def coProcess(grid, time, step):
    # initialize data description
    datadescription = vtkPVCatalyst.vtkCPDataDescription()
    datadescription.SetTimeData(time, step)
    datadescription.AddInput("input")
    cpscript.RequestDataDescription(datadescription)
    # to be continued ...

# Begin demo

from dolfin import *

# Print log messages only from the root process in parallel
parameters["std_out_all_processes"] = False;

# Load mesh from file
mesh = Mesh(DOLFIN_EXAMPLE_DATA_DIR+"/lshape.xml.gz")

# Define function spaces (P2-P1)
V = VectorFunctionSpace(mesh, "Lagrange", 2)
Q = FunctionSpace(mesh, "Lagrange", 1)

# Define trial and test functions
u = TrialFunction(V)
p = TrialFunction(Q)
v = TestFunction(V)
q = TestFunction(Q)

# Set parameter values
dt = 0.01
T = 3
nu = 0.01

# Define time-dependent pressure boundary condition
p_in = Expression("sin(3.0*t)", t=0.0)

# Define boundary conditions
noslip  = DirichletBC(V, (0, 0),
                      "on_boundary && \
                       (x[0] < DOLFIN_EPS | x[1] < DOLFIN_EPS | \
                       (x[0] > 0.5 - DOLFIN_EPS && x[1] > 0.5 - DOLFIN_EPS))")
inflow  = DirichletBC(Q, p_in, "x[1] > 1.0 - DOLFIN_EPS")
outflow = DirichletBC(Q, 0, "x[0] > 1.0 - DOLFIN_EPS")
bcu = [noslip]
bcp = [inflow, outflow]

# Create functions
u0 = Function(V)
u1 = Function(V)
p1 = Function(Q)

# Define coefficients
k = Constant(dt)
f = Constant((0, 0))

# Tentative velocity step
F1 = (1/k)*inner(u - u0, v)*dx + inner(grad(u0)*u0, v)*dx + \
     nu*inner(grad(u), grad(v))*dx - inner(f, v)*dx
a1 = lhs(F1)
L1 = rhs(F1)

# Pressure update
a2 = inner(grad(p), grad(q))*dx
L2 = -(1/k)*div(u1)*q*dx

# Velocity update
a3 = inner(u, v)*dx
L3 = inner(u1, v)*dx - k*inner(grad(p1), v)*dx

# Assemble matrices
A1 = assemble(a1)
A2 = assemble(a2)
A3 = assemble(a3)

# Use amg preconditioner if available
prec = "amg" if has_krylov_solver_preconditioner("amg") else "default"

# Create files for storing solution
ufile = File("results/velocity.pvd")
pfile = File("results/pressure.pvd")

# Time-stepping
maxtimestep = int(sys.argv[2])
tstep = 0
t = dt
while tstep < maxtimestep:

    # Update pressure boundary condition
    p_in.t = t

    # Compute tentative velocity step
    begin("Computing tentative velocity")
    b1 = assemble(L1)
    [bc.apply(A1, b1) for bc in bcu]
    solve(A1, u1.vector(), b1, "gmres", "default")
    end()

    # Pressure correction
    begin("Computing pressure correction")
    b2 = assemble(L2)
    [bc.apply(A2, b2) for bc in bcp]
    solve(A2, p1.vector(), b2, "gmres", prec)
    end()

    # Velocity correction
    begin("Computing velocity correction")
    b3 = assemble(L3)
    [bc.apply(A3, b3) for bc in bcu]
    solve(A3, u1.vector(), b3, "gmres", "default")
    end()

    # Plot solution [SC14-Catalyst] Not anymore
    # plot(p1, title="Pressure", rescale=True)
    # plot(u1, title="Velocity", rescale=True)

    # Save to file [SC14-Catalyst] Not anymore
    # ufile << u1
    # pfile << p1

    # [SC14-Catalyst] convert solution to VTK grid
    ugrid = None

    # [SC14-Catalyst] trigger catalyst execution
    coProcess(ugrid,t,tstep)

    # Move to next time step
    u0.assign(u1)
    t += dt
    tstep += 1
    print "t =", t, "step =",tstep

# Hold plot [SC14-Catalyst] Not anymore
# interactive()
