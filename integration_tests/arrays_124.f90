program arrays_124
    implicit none
    real :: buffer(3, 4)
    integer :: i

    buffer = -1.0
    call fill_rank3(2, 2, buffer)

    do i = 1, 4
        if (buffer(1, i) /= real(i)) error stop "bad first row"
        if (buffer(2, i) /= real(10 + i)) error stop "bad second row"
        if (buffer(3, i) /= real(20 + i)) error stop "bad third row"
    end do

contains

    subroutine fill_rank3(nj, ni, arr)
        integer, intent(in) :: nj, ni
        real, intent(out) :: arr(3, nj, ni)
        integer :: i, j

        do i = 1, ni
            do j = 1, nj
                arr(1, j, i) = real(j + nj*(i - 1))
                arr(2, j, i) = real(10 + j + nj*(i - 1))
                arr(3, j, i) = real(20 + j + nj*(i - 1))
            end do
        end do
    end subroutine

end program
