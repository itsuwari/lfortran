module c_backend_private_hidden_visibility_02_mod
implicit none
private
public :: run_hidden_array

contains

subroutine helper(a, total)
real(8), intent(in) :: a(:, :)
real(8), intent(inout) :: total

total = total + a(1, 1) + a(size(a, 1), size(a, 2))

end subroutine

subroutine run_hidden_array(total)
real(8), intent(inout) :: total
real(8) :: x(2, 2)

x = reshape([1.0d0, 2.0d0, 3.0d0, 4.0d0], [2, 2])
call helper(x, total)

end subroutine

end module

program c_backend_private_hidden_visibility_02
use c_backend_private_hidden_visibility_02_mod, only: run_hidden_array
implicit none
real(8) :: total

total = 0.0d0
call run_hidden_array(total)
if (abs(total - 5.0d0) > 1.0d-12) error stop

end program
