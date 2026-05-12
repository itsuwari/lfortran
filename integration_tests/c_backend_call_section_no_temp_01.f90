module c_backend_call_section_no_temp_01_m
implicit none
contains

subroutine consume(x, s)
real(8), intent(in) :: x(:, :)
real(8), intent(out) :: s
integer :: i, j

s = 0.0d0
do j = 1, size(x, 2)
    do i = 1, size(x, 1)
        s = s + x(i, j)
    end do
end do
end subroutine consume

subroutine bump(x)
real(8), intent(inout) :: x(:, :)

x = x + 2.0d0
end subroutine bump

end module c_backend_call_section_no_temp_01_m

program c_backend_call_section_no_temp_01
use c_backend_call_section_no_temp_01_m
implicit none

real(8) :: a(4, 5), s

a = 1.0d0
call consume(a(1:4:2, 2:4), s)

if (abs(s - 6.0d0) > 1.0d-12) error stop

a = 0.0d0
call bump(a(1:4:2, 2:4))

if (abs(sum(a(1:4:2, 2:4)) - 12.0d0) > 1.0d-12) error stop
if (abs(sum(a(2:4:2, :))) > 1.0d-12) error stop
if (abs(sum(a(:, 1))) > 1.0d-12) error stop
if (abs(sum(a(:, 5))) > 1.0d-12) error stop

end program c_backend_call_section_no_temp_01
