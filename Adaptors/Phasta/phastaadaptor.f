c$$$=========================================================================
c$$$
c$$$  Program:   ParaView
c$$$  Module:    phastaadaptor.f
c$$$
c$$$  Copyright (c) Kitware, Inc.
c$$$  All rights reserved.
c$$$  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
c$$$
c$$$     This software is distributed WITHOUT ANY WARRANTY; without even
c$$$     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
c$$$     PURPOSE.  See the above copyright notice for more information.
c$$$
c$$$=========================================================================


c...==============================================================
c... subroutine to do the coprocessing
c... The subroutine is responsible for determining if coprocessing
c... is needed this timestep and if it is needed then the
c... subroutine passes the phasta data structures into
c... the coprocessor. This is meant to be called at the end of
c... every time step.
c... The input is:
c...    itimestep -- the current simulation time step
c...    X -- the coordinates array of the nodes
c...    Y -- the fields array (e.g. velocity, pressure, etc.)
c...    compressibleflow -- flag to indicate whether or not the
c...                         flow is compressible.  if it is then
c...                         temperature will be outputted
c... It has no output and should not change any Phasta data.
c...==============================================================

      subroutine phastacoprocessor(itimestep, X, Y, compressibleflow)
      use pointer_data
      include "common.h"
      integer iblk, nenl, npro, j, needflag
      integer compressibleflow, itimestep
      dimension x(numnp,nsd), y(nshg,ndof)

      if(nshg .ne. numnp) then
         print *, 'CoProcessing only setup for when nshg equals numnp'
         return
      endif
c  First check if we even need to coprocess this time step/time
      call requestdatadescription(itimestep, time, needflag)
      if(needflag .eq. 0) then
c  We don't need to do any coprocessing now so we can return
         return
      endif

c  Check if we need to create the grid for the coprocessor
      call needtocreategrid(needflag)
      if(needflag .ne. 0) then
c     We do need the grid.
         call createpointsandallocatecells(numnp, X, numel)

        do iblk=1,nelblk
            nenl = lcblk(5,iblk) ! no. of vertices per element
            npro = lcblk(1,iblk+1) - lcblk(1,iblk) ! no. of elements in block
            call insertblockofcells(npro, nenl, mien(iblk)%p(1,1))
        enddo
      endif ! if needflag .ne. 0 --

c  Inside addfields we check to see if we really need the field or not
      call addfields(nshg, ndof, Y, compressibleflow)

      call coprocess()

      return
      end
