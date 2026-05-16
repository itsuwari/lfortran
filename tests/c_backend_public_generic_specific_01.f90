module c_backend_public_generic_specific_01_mod
implicit none
private
public :: apply

interface apply
    module procedure apply_r8
end interface

contains

subroutine apply_r8(a, total)
real(8), intent(in) :: a(:, :)
real(8), intent(inout) :: total

total = total + a(1, 1) + a(size(a, 1), size(a, 2))

end subroutine

end module

program c_backend_public_generic_specific_01
use c_backend_public_generic_specific_01_mod, only: apply
implicit none
real(8) :: x(2, 2), total

x = reshape([1.0d0, 2.0d0, 3.0d0, 4.0d0], [2, 2])
total = 0.0d0
call apply(x, total)
if (abs(total - 5.0d0) > 1.0d-12) error stop

end program
