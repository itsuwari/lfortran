module c_backend_nested_matmul_auto_temp_01_m
implicit none

real(8), parameter :: p(3, 3) = reshape([ &
    1.0_8, 0.0_8, 0.0_8, &
    0.0_8, 1.0_8, 0.0_8, &
    0.0_8, 0.0_8, 1.0_8], [3, 3])

contains

subroutine transform(a, c)
real(8), intent(in) :: a(:, :)
real(8), intent(out) :: c(:, :)

c = matmul(p, matmul(a, p))
end subroutine

end module

program c_backend_nested_matmul_auto_temp_01
use c_backend_nested_matmul_auto_temp_01_m, only: transform
implicit none

real(8) :: a(3, 3), c(3, 3)

a(1, 1) = 1.0_8; a(2, 1) = 2.0_8; a(3, 1) = 3.0_8
a(1, 2) = 4.0_8; a(2, 2) = 5.0_8; a(3, 2) = 6.0_8
a(1, 3) = 7.0_8; a(2, 3) = 8.0_8; a(3, 3) = 9.0_8

call transform(a, c)

if (abs(c(1, 1) - 1.0_8) > 1.0e-12_8) error stop
if (abs(c(2, 1) - 2.0_8) > 1.0e-12_8) error stop
if (abs(c(3, 1) - 3.0_8) > 1.0e-12_8) error stop
if (abs(c(1, 2) - 4.0_8) > 1.0e-12_8) error stop
if (abs(c(2, 2) - 5.0_8) > 1.0e-12_8) error stop
if (abs(c(3, 2) - 6.0_8) > 1.0e-12_8) error stop
if (abs(c(1, 3) - 7.0_8) > 1.0e-12_8) error stop
if (abs(c(2, 3) - 8.0_8) > 1.0e-12_8) error stop
if (abs(c(3, 3) - 9.0_8) > 1.0e-12_8) error stop
end program
