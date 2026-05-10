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

end module c_backend_call_section_no_temp_01_m

program c_backend_call_section_no_temp_01
use c_backend_call_section_no_temp_01_m
implicit none

real(8) :: a(4, 5), s

a = 1.0d0
call consume(a(:, 2:4), s)

if (abs(s - 12.0d0) > 1.0d-12) error stop

end program c_backend_call_section_no_temp_01
