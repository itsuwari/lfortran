program c_backend_reshape_order_section_01
implicit none

integer, parameter :: lx(3, 4) = reshape([ &
    0, 0, 0, &
    1, 2, 3, &
    4, 5, 6, &
    7, 8, 9], shape(lx), order=[2, 1])

call check_column(lx(:, 1), [0, 2, 6])
call check_column(lx(:, 2), [0, 3, 7])
call check_row(lx(1, :), [0, 0, 0, 1])
call check_row(lx(2, :), [2, 3, 4, 5])
call check_row(lx(3, :), [6, 7, 8, 9])

contains

subroutine check_column(actual, expected)
    integer, intent(in) :: actual(:)
    integer, intent(in) :: expected(3)
    integer :: i

    do i = 1, 3
        if (actual(i) /= expected(i)) error stop i
    end do
end subroutine

subroutine check_row(actual, expected)
    integer, intent(in) :: actual(:)
    integer, intent(in) :: expected(4)
    integer :: i

    do i = 1, 4
        if (actual(i) /= expected(i)) error stop i
    end do
end subroutine

end program
