! Declaration of subroutines for Fortran Bindings
! open/close interface

      INTERFACE
        INTEGER*8 FUNCTION h5pt_openr ( filename )
          CHARACTER(LEN=*), INTENT(IN) :: filename      ! the filename to open for reading
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_openw ( filename )
          CHARACTER(LEN=*), INTENT(IN) :: filename      ! the filename to open for writing
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_opena ( filename )
          CHARACTER(LEN=*), INTENT(IN) :: filename      ! the filename to open for appending
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_openr_par ( filename, mpi_communicator )
          CHARACTER(LEN=*), INTENT(IN) :: filename      ! the filename to open for reading
          INTEGER, INTENT(IN) :: mpi_communicator       ! the MPI_Communicator used by the program
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_openw_par ( filename, mpi_communicator )
          CHARACTER(LEN=*), INTENT(IN) :: filename      ! the filename to open for writing
          INTEGER, INTENT(IN) :: mpi_communicator       ! the MPI_Communicator used by the program
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_opena_par ( filename, mpi_communicator )
          CHARACTER(LEN=*), INTENT(IN) :: filename      ! the filename to open for appending
          INTEGER, INTENT(IN) :: mpi_communicator       ! the MPI_Communicator used by the program
        END FUNCTION


        INTEGER*8 FUNCTION h5pt_close ( filehandle )
          INTEGER*8, INTENT(IN) :: filehandle           ! close this open filehandle
        END FUNCTION

!==============Writing and Setting Dataset info========

        INTEGER*8 FUNCTION h5pt_setnpoints ( filehandle, npoints )
          INTEGER*8, INTENT(IN) :: filehandle
          INTEGER*8, INTENT(IN) :: npoints              ! The number of particles on *this* processor
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_setstep (filehandle,step)
          INTEGER*8, INTENT(IN) :: filehandle
          INTEGER*8, INTENT(IN) :: step                 ! Set the current timestep in the file to this
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_writedata_r8 ( filehandle, name, data )
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: name          ! The name of the data we are writing
          REAL*8, INTENT(IN) :: data(*)                 ! The dataarray to write.  The number of
                                                        ! elements is presumably set earlier with
                                                        ! h5pt_setnpoints(f,npoints)
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_writedata_i8 ( filehandle, name, data )
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: name          ! The name of the data we are writing
          INTEGER*8, INTENT(IN) :: data(*)
        END FUNCTION

!==============Reading Data Characteristics============

        INTEGER*8 FUNCTION h5pt_getnsteps (filehandle)
          INTEGER*8, INTENT(IN) :: filehandle
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_getndatasets (filehandle)
          INTEGER*8, INTENT(IN) :: filehandle
        END FUNCTION

!     returns total number of points in this timestep
!     If a "view" has been set using h5pt_setview()
!     then it returns the number of points that are
!     in the current view.
        INTEGER*8 FUNCTION h5pt_getnpoints (filehandle)
          INTEGER*8, INTENT(IN) :: filehandle
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_getdatasetname (filehandle,index,name)
          INTEGER*8, INTENT(IN) :: filehandle
          INTEGER*8, INTENT(IN) :: index                ! Index for a given dataset name
          CHARACTER(LEN=*), INTENT(OUT) :: name         ! returns the name of the dataset at that index
        END FUNCTION

!=============Setting and getting views================
        INTEGER*8 FUNCTION h5pt_setview (filehandle,start,end)
          INTEGER*8, INTENT(IN) :: filehandle
          INTEGER*8, INTENT(IN) :: start                ! offset of the first particle in the view
          INTEGER*8, INTENT(IN) :: end                  ! offset of the first particle after the end of the view
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_resetview (filehandle)
          INTEGER*8, INTENT(IN) :: filehandle           ! reset the view on this filehandle to default
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_hasview (filehandle)
          INTEGER*8, INTENT(IN) :: filehandle
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_getview (filehandle,start,end)
          INTEGER*8, INTENT(IN) :: filehandle
          INTEGER*8, INTENT(OUT) :: start               ! offset of first particle in the view
          INTEGER*8, INTENT(OUT) :: end                 ! offset of first particle beyond the current view
        END FUNCTION

!==============Reading Data=========================

        INTEGER*8 FUNCTION h5pt_readdata_r8 (filehandle,name,data)
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: name          ! The name of the data we are writing
          REAL*8, INTENT(OUT) :: data(*)                ! The dataarray to read. Number of points
                                                        ! read is either the number within the view set
                                                        ! by h5pt_setview() or the default (the total
                                                        ! number of particles in the file.
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_readdata_i8 (filehandle,name,data)
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: name          ! The name of the data we are writing
          INTEGER*8, INTENT(OUT) :: data(*)             ! The dataarray to read. Number of points
                                                        ! read is either the number within the view set
                                                        ! by h5pt_setview() or the default (the total
                                                        ! number of particles in the file.
        END FUNCTION


!=================== Attributes ================

        INTEGER*8 FUNCTION h5pt_writefileattrib_r8 (filehandle,attrib_name,attrib_value,attrib_nelem)
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: attrib_name   ! The name of the attribute
          REAL*8, INTENT(IN) :: attrib_value(*)         ! The array of data to write into the attribute
          INTEGER*8, INTENT(IN) :: attrib_nelem         ! Number of elements in the attrib array
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_writefileattrib_i8 (filehandle,attrib_name,attrib_value,attrib_nelem)
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: attrib_name   ! The name of the attribute
          INTEGER*8, INTENT(IN) :: attrib_value(*)      ! The array of data to write into the attribute
          INTEGER*8, INTENT(IN) :: attrib_nelem         ! Number of elements in the attrib array
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_writefileattrib_string (filehandle,attrib_name,attrib_value)
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: attrib_name   ! The name of the attribute
          CHARACTER(LEN=*), INTENT(IN) :: attrib_value  ! The array of data to write into the attribute
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_writestepattrib_r8 (filehandle,attrib_name,attrib_value,attrib_nelem)
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: attrib_name   ! The name of the attribute
          REAL*8, INTENT(IN) :: attrib_value(*)         ! The array of data to write into the attribute
          INTEGER*8, INTENT(IN) :: attrib_nelem         ! Number of elements in the attrib array
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_writestepattrib_i8 (filehandle,attrib_name,attrib_value,attrib_nelem)
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: attrib_name   ! The name of the attribute
          INTEGER*8, INTENT(IN) :: attrib_value(*)      ! The array of data to write into the attribute
          INTEGER*8, INTENT(IN) :: attrib_nelem         ! Number of elements in the attrib array
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_writestepattrib_string (filehandle,attrib_name,attrib_value)
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: attrib_name   ! The name of the attribute
          CHARACTER(LEN=*), INTENT(IN) :: attrib_value  ! The array of data to write into the attribute
        END FUNCTION


        INTEGER*8 FUNCTION h5pt_getnstepattribs (filehandle)
          INTEGER*8, INTENT(IN) :: filehandle
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_getnfileattribs (filehandle)
          INTEGER*8, INTENT(IN) :: filehandle
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_getstepattribinfo (filehandle,idx,attrib_name,attrib_nelem)
          INTEGER*8, INTENT(IN) :: filehandle
          INTEGER*8, INTENT(IN) :: idx                  ! index of the attribute being queried
          CHARACTER(LEN=*), INTENT(OUT) :: attrib_name  ! The name of the attribute
          INTEGER*8, INTENT(OUT) :: attrib_nelem        ! Number of elements in the attrib array
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_getfileattribinfo (filehandle,idx,attrib_name,attrib_nelem)
          INTEGER*8, INTENT(IN) :: filehandle
          INTEGER*8, INTENT(IN) :: idx                  ! index of the attribute being queried
          CHARACTER(LEN=*), INTENT(OUT) :: attrib_name  ! The name of the attribute
          INTEGER*8, INTENT(OUT) :: attrib_nelem        ! Number of elements in the attrib array
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_readstepattrib_i8 ( filehandle, attrib_name, attrib_value )
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: attrib_name   ! name of the attribute to read
          INTEGER*8, INTENT(OUT) :: attrib_value(*)     ! the attribute data will be read into this array
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_readstepattrib_r8 ( filehandle, attrib_name, attrib_value )
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: attrib_name   ! name of the attribute to read
          REAL*8, INTENT(OUT) :: attrib_value(*)        ! the attribute data will be read into this array
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_readfileattrib_i8 (filehandle, attrib_name, attrib_value )
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: attrib_name   ! name of the attribute to read
          INTEGER*8, INTENT(OUT) :: attrib_value(*)     ! the attribute data will be read into this array
        END FUNCTION

        INTEGER*8 FUNCTION h5pt_readfileattrib_r8 (filehandle, attrib_name, attrib_value )
          INTEGER*8, INTENT(IN) :: filehandle
          CHARACTER(LEN=*), INTENT(IN) :: attrib_name   ! name of the attribute to read
          REAL*8, INTENT(OUT) :: attrib_value(*)        ! the attribute data will be read into this array
        END FUNCTION


 INTEGER*8 FUNCTION h5pt_set_verbosity_level ( level )
   INTEGER*8, INTENT(IN) :: level
 END FUNCTION

      END INTERFACE
