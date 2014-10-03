module Box
  implicit none
  public :: getownedbox, getlocalbox

contains

  ! this gives a partitioning of an extent based on its inputs.
  ! the partitioning will overlap (e.g. 0 to 10 partitioned into
  ! 2 pieces would result in 0-5 and 5-10). arguments are:
  ! piece: which piece to get the partition for (between 1 and numpieces)
  ! numpieces: the number of pieces that the extent is partitioned into
  ! globalbox: the extent where the values are ordered by
  !  {max x index, max y index, max z index}
  ! box: the returned local extent where the values are ordered by
  !  {min x index, max x index, min y index, max y index, min z index, max z index}
  subroutine getlocalbox(piece, numpieces, dimensions, box)
    implicit none
    integer, intent(in) :: piece, numpieces, dimensions(3)
    integer, intent(inout) :: box(6)
    integer :: numpiecesinfirsthalf, splitaxis, mid, cnt
    integer :: numpieceslocal, piecelocal, i, size(3)

    do i=1, 3
       box((i-1)*2+1) = 1
       box(2*i) = dimensions(i)
    enddo

    if (piece .gt. numpieces .or. piece .lt. 0) return
    ! keep splitting until we have only one piece.
    ! piece and numpieces will always be relative to the current box.
    cnt = 0
    numpieceslocal = numpieces
    piecelocal = piece-1
    do while (numpieceslocal .gt. 1)
       size(1) = box(2) - box(1)
       size(2) = box(4) - box(3)
       size(3) = box(6) - box(5)
       ! choose what axis to split on based on the SplitMode
       ! if the user has requested x, y, or z slabs then try to
       ! honor that request. If that axis is already split as
       ! far as it can go, then drop to block mode.
       ! choose the biggest axis
       if (size(3) .ge. size(2) .and. size(3) .ge. size(1) .and. size(3)/2 .ge. 1) then
          splitaxis = 3
       else if (size(2) .ge. size(1) .and. size(2)/2 .ge. 1) then
          splitaxis = 2
       else if (size(1)/2 .ge. 1) then
          splitaxis = 1
       else
          splitaxis = -1
       endif

       if (splitaxis .eq. -1) then
          ! can not split any more.
          if (piecelocal .eq. 0) then
             ! just return the remaining piece
             numpieceslocal = 1
          else
             ! the rest must be empty
             return
          endif
       else ! (splitaxis .eq. -1)
          ! split the chosen axis into two pieces.
          numpiecesinfirsthalf = (numpieceslocal / 2)
          mid = size(splitaxis)
          mid = (mid *  numpiecesinfirsthalf) / numpieceslocal + box((splitaxis-1)*2+1)
          if (piecelocal .lt. numpiecesinfirsthalf) then
             ! piece is in the first half
             ! set boxent to the first half of the previous value.
             box((splitaxis-1)*2+2) = mid
             ! piece must adjust.
             numpieceslocal = numpiecesinfirsthalf
          else
             ! piece is in the second half.
             ! set the boxent to be the second half. (two halves share points)
             box((splitaxis-1)*2+1) = mid
             ! piece must adjust
             numpieceslocal = numpieceslocal - numpiecesinfirsthalf
             piecelocal = piecelocal - numpiecesinfirsthalf
          endif
       endif
    end do
    return
  end subroutine getlocalbox

  ! box is only locally owned on the minimum side of that
  ! is at the domain boundary
  subroutine getownedbox(piece, numpieces, dimensions, ownedbox)
    implicit none
    integer, intent(in) :: dimensions(3), piece, numpieces
    integer, intent(out) :: ownedbox(6)
    integer :: i, localbox(6)

    call getlocalbox(piece, numpieces, dimensions, localbox)

    do i=1, 3
       ! minimums
       if(localbox((i-1)*2+1) .eq. 1) then
          ownedbox((i-1)*2+1) = 1
       else
          if(localbox((i-1)*2+1) .ne. dimensions(i)) then
             ownedbox((i-1)*2+1) = localbox((i-1)*2+1)+1
          else
             ! this happens when the domain has a single point in this direction
             ownedbox((i-1)*2+1) = 1
          endif
       endif
       ! maximums
       ownedbox(i*2) = localbox(i*2)
    end do

  end subroutine getownedbox

end module Box
