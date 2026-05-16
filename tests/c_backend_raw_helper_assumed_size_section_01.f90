module c_backend_raw_helper_assumed_size_section_01_m
implicit none
private
public :: run

contains

subroutine gen(num, x, w, v)
integer, intent(inout) :: num
real(8), intent(inout) :: x(3, *)
real(8), intent(inout) :: w(*)
real(8), intent(in) :: v

x(:, 1) = [v, 0.0d0, -v]
w(1:3) = v
num = num + 3
end subroutine gen

subroutine run()
real(8) :: x(3, 6), w(6)
integer :: n

x = 0.0d0
w = 0.0d0
n = 1
call gen(n, x(1, n), w(n), 2.5d0)
if (n /= 4) error stop
if (abs(x(1, 1) - 2.5d0) > 1.0d-12) error stop
if (abs(x(2, 1)) > 1.0d-12) error stop
if (abs(x(3, 1) + 2.5d0) > 1.0d-12) error stop
if (abs(sum(w(1:3)) - 7.5d0) > 1.0d-12) error stop
end subroutine run

end module c_backend_raw_helper_assumed_size_section_01_m

program c_backend_raw_helper_assumed_size_section_01
use c_backend_raw_helper_assumed_size_section_01_m, only: run
implicit none

call run()
end program c_backend_raw_helper_assumed_size_section_01
