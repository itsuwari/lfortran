module c_backend_section_assoc_temp_direct_01_m
implicit none

contains

subroutine copy_plane(a, b)
real(8), intent(in) :: a(:, :)
real(8), intent(out) :: b(:, :)

b(1, 1) = a(1, 1)
b(2, 1) = a(2, 1)

end subroutine

subroutine copy_planes(a, b)
real(8), intent(in) :: a(:, :, :)
real(8), intent(out) :: b(:, :, :)
integer :: k

do k = 1, size(a, 1)
    call copy_plane(a(k, :, :), b(k, :, :))
end do

end subroutine

end module

program c_backend_section_assoc_temp_direct_01
use c_backend_section_assoc_temp_direct_01_m, only: copy_planes
implicit none

real(8) :: a(2, 2, 1), b(2, 2, 1)

a = 0.0d0
b = 0.0d0
a(1, 1, 1) = 11.0d0
a(2, 1, 1) = 12.0d0

call copy_planes(a, b)

if (abs(b(1, 1, 1) - 11.0d0) > 1.0d-12) error stop
if (abs(b(2, 1, 1) - 12.0d0) > 1.0d-12) error stop

end program
