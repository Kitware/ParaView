subroutine getlocalbox(piece, numpieces, box)
  implicit none
  integer, intent(in) :: piece, numpieces
  integer, intent(inout) :: box(6)
  integer numPiecesInFirstHalf, size(3), splitAxis, mid, cnt
  if (piece .gt. numPieces .or. piece .lt. 0) return
  ! keep splitting until we have only one piece.
  ! piece and numPieces will always be relative to the current ext.
  cnt = 0
  do while (numPieces .gt. 1)
    ! Get the dimensions for each axis.
    size(0) = ext(1)-ext(0);
    size(1) = ext(3)-ext(2);
    size(2) = ext(5)-ext(4);
    ! choose what axis to split on based on the SplitMode
    ! if the user has requested x, y, or z slabs then try to
    ! honor that request. If that axis is already split as
    ! far as it can go, then drop to block mode.
    ! choose the biggest axis
    if (size(2) .ge. size(1) .and size(2) .ge. size(0) .and. size(2)/2 .ge. 1) then
       splitAxis = 2
    else if (size(1) .ge. size(0) .and. size(1)/2 .ge. 1) then
       splitAxis = 1
    else if (size(0)/2 .ge. 1) then
       splitAxis = 0
    else
       splitAxis = -1
    endif

    if (splitAxis .eq. -1) then
      ! can not split any more.
      if (piece .eq. 0) then
        ! just return the remaining piece
        numPieces = 1;
      else
        ! the rest must be empty
        return
     endif
  else ! (splitAxis .eq. -1)
     ! split the chosen axis into two pieces.
     numPiecesInFirstHalf = (numPieces / 2);
     mid = size(splitAxis);
     mid = (mid *  numPiecesInFirstHalf) / numPieces + ext(splitAxis*2);
     if (piece .lt. numPiecesInFirstHalf) then
        ! piece is in the first half
        ! set extent to the first half of the previous value.
        ext(splitAxis*2+1) = mid
        ! piece must adjust.
        numPieces = numPiecesInFirstHalf
     else
        ! piece is in the second half.
        ! set the extent to be the second half. (two halves share points)
        ext(splitAxis*2) = mid
        ! piece must adjust
        numPieces = numPieces - numPiecesInFirstHalf;
        piece -= numPiecesInFirstHalf;
     endif
  end do
  return

