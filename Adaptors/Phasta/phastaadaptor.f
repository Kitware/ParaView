c$$$ SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
c$$$ SPDX-License-Identifier: BSD-3-Clause

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
      use catalyst
      include "common.h"
      integer iblk, nenl, npro, j
      logical needflag
      integer compressibleflow, itimestep
      dimension x(numnp,nsd), y(nshg,ndof)

      if(nshg .ne. numnp) then
         print *, 'CoProcessing only setup for when nshg equals numnp'
         return
      endif
c  First check if we even need to coprocess this time step/time
      needflag = catalyst_request_data_description(itimestep, time)
      if(not needflag) then
c  We don't need to do any coprocessing now so we can return
         return
      endif

c  Check if we need to create the grid for the coprocessor
      needflag = catalyst_need_to_create_grid()
      if(needflag) then
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

      call catalyst_process()

      return
      end
