module c_backend_raw_array_helper_call_01_mod
implicit none
private
public :: fill_raw_helper_case

contains

subroutine fill_raw_helper_case(x, w)
real(8), intent(inout) :: x(:, :)
real(8), intent(inout) :: w(:)
integer :: n

n = 2
call gen(n, x(1, n), w(n), 3.0d0)

end subroutine

subroutine gen(num, x, w, v)
integer, intent(inout) :: num
real(8), intent(inout) :: x(2, *)
real(8), intent(inout) :: w(*)
real(8), intent(in) :: v

x(1, 1) = v
x(2, 1) = v + 1.0d0
w(1) = v + 2.0d0
num = num + 1

end subroutine

end module

program c_backend_raw_array_helper_call_01
use c_backend_raw_array_helper_call_01_mod, only: fill_raw_helper_case
implicit none
real(8) :: x(2, 4), w(4)

x = 0.0d0
w = 0.0d0
call fill_raw_helper_case(x, w)

if (abs(x(1, 2) - 3.0d0) > 1.0d-12) error stop
if (abs(x(2, 2) - 4.0d0) > 1.0d-12) error stop
if (abs(w(2) - 5.0d0) > 1.0d-12) error stop
if (abs(x(1, 1)) > 1.0d-12) error stop
if (abs(w(1)) > 1.0d-12) error stop

end program
