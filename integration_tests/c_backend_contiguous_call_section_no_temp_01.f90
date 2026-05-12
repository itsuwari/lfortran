module c_backend_contiguous_call_section_no_temp_01_m
implicit none

contains

subroutine consume_contiguous(x, s)
    real(8), contiguous, intent(in) :: x(:, :)
    real(8), intent(out) :: s
    integer :: i, j

    s = 0.0d0
    do j = 1, size(x, 2)
        do i = 1, size(x, 1)
            s = s + x(i, j)
        end do
    end do
end subroutine consume_contiguous

real(8) function section_total(x)
    real(8), intent(in) :: x(:, :)
    integer :: i, j

    section_total = 0.0d0
    do j = 1, size(x, 2)
        do i = 1, size(x, 1)
            section_total = section_total + x(i, j)
        end do
    end do
end function section_total

subroutine bump_contiguous(x)
    real(8), contiguous, intent(inout) :: x(:, :)

    x = x + 2.0d0
end subroutine bump_contiguous

end module c_backend_contiguous_call_section_no_temp_01_m

program c_backend_contiguous_call_section_no_temp_01
use c_backend_contiguous_call_section_no_temp_01_m
implicit none

real(8) :: a(4, 5), s

a = 1.0d0
call consume_contiguous(a(:, 2:4), s)

if (abs(s - 12.0d0) > 1.0d-12) error stop

a = 0.0d0
call bump_contiguous(a(:, 2:4))

if (abs(section_total(a(:, 2:4)) - 24.0d0) > 1.0d-12) error stop
if (abs(section_total(a(:, 1:1))) > 1.0d-12) error stop
if (abs(section_total(a(:, 5:5))) > 1.0d-12) error stop

end program c_backend_contiguous_call_section_no_temp_01
