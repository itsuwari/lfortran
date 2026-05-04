program c_backend_lazy_matmul_temp_01
implicit none

real(8) :: a(3, 3), b(3, 3), c(3, 3)
integer :: i, j

do j = 1, 3
    do i = 1, 3
        a(i, j) = real(i + 10*j, 8)
        b(i, j) = real(i - j, 8)
        c(i, j) = 0.0_8
    end do
end do

call transform(0, a, b, c)
if (abs(c(2, 3) - a(2, 3)) > 1.0e-12_8) error stop

call transform(1, a, b, c)
if (abs(c(2, 3) - sum(a(2, :)*b(:, 3))) > 1.0e-12_8) error stop

contains

subroutine transform(mode, a, b, c)
integer, intent(in) :: mode
real(8), intent(in) :: a(:, :), b(:, :)
real(8), intent(out) :: c(:, :)

select case (mode)
case (0)
    c(:, :) = a(:, :)
case default
    c(:, :) = matmul(a, b)
end select
end subroutine

end program
