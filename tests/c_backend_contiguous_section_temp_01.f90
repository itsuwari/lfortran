module c_backend_contiguous_section_temp_01_m
contains
subroutine consume(x, s)
    real(8), contiguous, intent(in) :: x(:, :)
    real(8), intent(out) :: s

    s = sum(x)
end subroutine consume
end module c_backend_contiguous_section_temp_01_m

program c_backend_contiguous_section_temp_01
use c_backend_contiguous_section_temp_01_m
implicit none

real(8) :: a(4, 5), s
integer :: i, j

do j = 1, 5
    do i = 1, 4
        a(i, j) = 10*j + i
    end do
end do

call consume(a(1:4:2, 2:4), s)

if (abs(s - 192.0d0) > 1.0d-12) error stop
end program c_backend_contiguous_section_temp_01
