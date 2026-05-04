program c_backend_rank2_full_self_update_01
implicit none

real(8), allocatable :: a(:, :), b(:, :), c(:, :)
real(8) :: d(3, 3), scale
integer :: i, j

allocate(a(3, 3), b(3, 3), c(3, 3))
scale = 3.0_8

do j = 1, 3
    do i = 1, 3
        a(i, j) = real(i + 10*j, 8)
        b(i, j) = real(i + j, 8)
        c(i, j) = real(100 + i + 10*j, 8)
        d(i, j) = real(2*i + j, 8)
    end do
end do

a(:, :) = a + b*2.0_8
a(:, :) = a + d*scale
c(2:3, :2) = c(2:3, :2) + b(2:3, :2)

do j = 1, 3
    do i = 1, 3
        if (abs(a(i, j) - real(i + 10*j + 2*(i + j) + 3*(2*i + j), 8)) > 1.0e-12_8) error stop
        if (i >= 2 .and. j <= 2) then
            if (abs(c(i, j) - real(100 + i + 10*j + i + j, 8)) > 1.0e-12_8) error stop
        else
            if (abs(c(i, j) - real(100 + i + 10*j, 8)) > 1.0e-12_8) error stop
        end if
    end do
end do

call check_scalar_index_section_self_update()

contains

subroutine check_scalar_index_section_self_update()
    real(8) :: x(3, 2, 3, 2), y(3, 3), z(3, 3)

    x = 1.0_8
    y = 2.0_8
    z = 3.0_8
    call update_plane(x, y, z, 4.0_8, 2)
    if (abs(x(2, 1, 3, 1) - 21.0_8) > 1.0e-12_8) error stop
end subroutine

subroutine update_plane(x, y, z, scale, n)
    integer, intent(in) :: n
    real(8), intent(inout) :: x(3, n, 3, n)
    real(8), intent(in) :: y(3, 3), z(3, 3), scale
    integer :: ii, jj

    ii = 1
    jj = 1
    x(:, jj, :, ii) = x(:, jj, :, ii) + scale*(y + z)
end subroutine

end program
