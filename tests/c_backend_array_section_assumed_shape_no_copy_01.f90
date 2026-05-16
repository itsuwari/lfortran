module c_backend_array_section_assumed_shape_no_copy_01_mod
implicit none
contains

subroutine consume(a, n)
    integer, intent(in) :: n
    real(8), intent(in) :: a(:, :)

    if (size(a, 1) /= n) error stop
    if (size(a, 2) /= n) error stop
    if (abs(a(1, 1) - 11.0d0) > 1.0d-12) error stop
    if (abs(a(n, n) - 22.0d0) > 1.0d-12) error stop
end subroutine

end module

program c_backend_array_section_assumed_shape_no_copy_01
use c_backend_array_section_assumed_shape_no_copy_01_mod, only: consume
implicit none

real(8), allocatable :: x(:, :)

allocate(x(0:3, -1:2))
x = 0.0d0
x(1, 0) = 11.0d0
x(2, 1) = 22.0d0
call consume(x(1:2, 0:1), 2)

end program
