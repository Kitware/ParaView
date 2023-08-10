! SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
! SPDX-License-Identifier: BSD-3-Clause

module catalyst_adaptor
  use, intrinsic :: iso_c_binding, only: C_PTR
  use, intrinsic :: iso_fortran_env, only: stderr => error_unit
  use catalyst_conduit
  use catalyst_api

  implicit none

contains

  subroutine catalyst_adaptor_initialize()

    integer :: argc = 0
    integer :: i = 0
    character(len=4096) :: arg, tmp, script_name
    type(C_PTR) node, script_args_item, script_args
    integer(kind(catalyst_status)) :: err

    ! This node will hold the information nessesary to initializa
    ! ParaViewCatalyst along with the python script and its arguments
    ! The target structure for this example is:
    !
    ! catalyst:
    !   scripts:
    !     script1:
    !       filename: arg[1]
    !       args:
    !         - "--channel-name=grid"
    ! catalyst_load:
    !   implementation: "paraview"
    !   search_paths:
    !     paraview: `PARAVIEW_IMPL_DIR` ( to be replaced during compilation)
    !
    ! For a complete description of the initialize protocol see
    ! https://docs.paraview.org/en/latest/Catalyst/blueprints.html#protocol-initialize
    node = catalyst_conduit_node_create()

    ! First argument is the catalyst script name , every subsequent  argument is passed to the script

    ! set script name
    i = 1
    call get_command_argument(i, arg)
    write (tmp, '(A,I1)') 'catalyst/scripts/script', i
    call catalyst_conduit_node_set_path_char8_str(node, trim(tmp)//"/filename", arg)

    ! object to keep the script arguments
    script_args = catalyst_conduit_node_fetch(node, trim(tmp)//"/args")

    ! add item manually to script_args list
    script_args_item = catalyst_conduit_node_append(script_args)
    ! set its value
    call catalyst_conduit_node_set_char8_str(script_args_item, "--channel-name=grid")

    !  Add the rest of the program arguments as script arguments
    argc = command_argument_count()
    do i = 2, argc
      call get_command_argument(i, tmp)
      script_args_item = catalyst_conduit_node_append(script_args)
      call catalyst_conduit_node_set_char8_str(script_args_item, tmp)
    end do

    ! Alternatively set the environmental variable CATALYST_IMPLEMENTATION_NAME to "paraview
    call catalyst_conduit_node_set_path_char8_str(node, "catalyst_load/implementation", "paraview")

    ! Alternatively set the environmental variable CATALYST_IMPLEMENTATION_PATHS to the path of libcatalyst-paraview.so
    call catalyst_conduit_node_set_path_char8_str(node, "catalyst_load/search_paths/paraview", &
                                                  PARAVIEW_IMPL_DIR)

    err = c_catalyst_initialize(node)
    if (err /= catalyst_status_ok) then
      write (stderr, *) "ERROR: Failed to initialize Catalyst: ", err
    end if

    !call catalyst_conduit_node_print(node)

    call catalyst_conduit_node_destroy(node)
  end subroutine

  subroutine catalyst_adaptor_execute(nxstart, nxend, nx, ny, nz, step, time, psi01)
    integer, parameter :: f64 = selected_real_kind(8)
    integer, intent(in) :: nxstart, nxend, nx, ny, nz, step
    real(kind=8), intent(in) :: time
    complex(kind=8), dimension(:, :, :), intent(in) :: psi01
    real(kind=8), allocatable  :: psi01_real(:, :, :)
    real(kind=8), allocatable :: psi01_imag(:, :, :)
    integer(kind(catalyst_status)) :: err

    type(C_PTR) catalyst_exec_params, mesh, channel, fields
    integer(8) :: npoints, spacingX, spacingY, spacingZ

    ! populate the node based on the "execute" protocol of ParaViewCatalyst
    ! For reference see https://docs.paraview.org/en/latest/Catalyst/blueprints.html#protocol-execute

    catalyst_exec_params = catalyst_conduit_node_create()

    call catalyst_conduit_node_set_path_int32(catalyst_exec_params, "catalyst/state/timestep", step)
    call catalyst_conduit_node_set_path_float64(catalyst_exec_params, "catalyst/state/time", time)

    ! the data must be provided on a named channel. the name is determined by the
    ! simulation. for this one, we're calling it "grid".

    ! Add channels.
    ! We only have 1 channel here. Let's name it 'grid'.
    ! equiv. C++ code:  auto channel = exec_params["catalyst/channels/grid"]
    channel = catalyst_conduit_node_fetch(catalyst_exec_params, "catalyst/channels/grid")

    ! Since this example is using Conduit Mesh Blueprint to define the mesh,
    ! we set the channel's type to "mesh".
    ! The the complete protocol reference see
    ! https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#mesh-blueprint

    ! eq. C++ code : channel["type"].set("mesh");
    call catalyst_conduit_node_set_path_char8_str(channel, "type", "mesh")

    ! now create the mesh.
    ! auto mesh = channel["data"];
    mesh = catalyst_conduit_node_fetch(channel, "data")

    ! start with coordsets (of course, the sequence is not important, just make
    ! it easier to think in this order).
    call catalyst_conduit_node_set_path_char8_str(mesh, "coordsets/coords/type", "uniform")

    call catalyst_conduit_node_set_path_int32(mesh, "coordsets/coords/dims/i", nxend - nxstart + 1)
    call catalyst_conduit_node_set_path_int32(mesh, "coordsets/coords/dims/j", ny)
    call catalyst_conduit_node_set_path_int32(mesh, "coordsets/coords/dims/k", nz)

    call catalyst_conduit_node_set_path_float64(mesh, "coordsets/coords/origin/x", real(nxstart, 8))
    call catalyst_conduit_node_set_path_float64(mesh, "coordsets/coords/origin/y", 0.0_f64)
    call catalyst_conduit_node_set_path_float64(mesh, "coordsets/coords/origin/z", 0.0_f64)

    call catalyst_conduit_node_set_path_int32(mesh, "coordsets/coords/spacing/dx", 1)
    call catalyst_conduit_node_set_path_int32(mesh, "coordsets/coords/spacing/dy", 1)
    call catalyst_conduit_node_set_path_int32(mesh, "coordsets/coords/spacing/dz", 1)

    ! Next, add topology
    call catalyst_conduit_node_set_path_char8_str(mesh, "topologies/mesh/type", "uniform")
    call catalyst_conduit_node_set_path_char8_str(mesh, "topologies/mesh/coordset", "coords")

    ! Finally, add fields.
    ! auto fields = mesh["fields"];
    fields = catalyst_conduit_node_fetch(mesh, "fields")
    call catalyst_conduit_node_set_path_char8_str(fields, "psi01/association", "vertex")
    call catalyst_conduit_node_set_path_char8_str(fields, "psi01/topology", "mesh")
    call catalyst_conduit_node_set_path_char8_str(fields, "psi01/volume_dependent", "false")

    ! psi01 is stored in non-interlaced form
    npoints = (nxend - nxstart + 1)*ny*nz

    ! ideally we would use conduit_node_set_path_external_float64_ptr_detailed but it is not yet included in a released version of conduit.
    allocate (psi01_real(nxend - nxstart + 1, ny, nz))
    allocate (psi01_imag(nxend - nxstart + 1, ny, nz))

    psi01_real = real(psi01)
    psi01_imag = aimag(psi01)

    call catalyst_conduit_node_set_path_external_float64_ptr(fields, "psi01/values/u", psi01_real, npoints)
    call catalyst_conduit_node_set_path_external_float64_ptr(fields, "psi01/values/v", psi01_imag, npoints)

    !call catalyst_conduit_node_print(mesh)

    err = c_catalyst_execute(catalyst_exec_params)
    if (err /= catalyst_status_ok) then
      write (stderr, *) "ERROR: Failed to execute Catalyst: ", err
    end if

    call catalyst_conduit_node_destroy(catalyst_exec_params)
    deallocate (psi01_real)
    deallocate (psi01_imag)

  end subroutine

  subroutine catalyst_adaptor_finalize()
    type(C_PTR) node
    integer(kind(catalyst_status)) :: err

    node = catalyst_conduit_node_create()
    err = c_catalyst_finalize(node)
    if (err /= catalyst_status_ok) then
      write (stderr, *) "ERROR: Failed to finalize Catalyst: ", err
    end if

    call catalyst_conduit_node_destroy(node)

  end subroutine
end module catalyst_adaptor
