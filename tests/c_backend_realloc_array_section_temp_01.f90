program c_backend_realloc_array_section_temp_01
implicit none

real(8) :: a(2, 2, 2), b(2, 2)
integer :: i, j, k

do k = 1, 2
    do j = 1, 2
        do i = 1, 2
            a(i, j, k) = real(i + 10*j + 100*k, 8)
        end do
    end do
end do
b = 10.0_8

call update(a, b)

if (abs(a(1, 1, 1) - 121.0_8) > 1e-12_8) error stop
if (abs(a(2, 2, 1) - 132.0_8) > 1e-12_8) error stop
if (abs(a(1, 1, 2) - 211.0_8) > 1e-12_8) error stop

contains

    subroutine update(x, y)
    real(8), intent(inout) :: x(:, :, :)
    real(8), intent(in) :: y(:, :)

    x(:, :, 1) = x(:, :, 1) + y
    end subroutine

end program
