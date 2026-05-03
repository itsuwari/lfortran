subroutine add_arrays(n, a, b, c)
implicit none

integer, intent(in) :: n
real(8), intent(in) :: a(n), b(n)
real(8), intent(out) :: c(n)
integer :: i

do i = 1, n
    c(i) = a(i) + b(i)
end do

end subroutine

program c_backend_dummy_array_restrict_01
implicit none

integer, parameter :: n = 3
real(8) :: a(n), b(n), c(n)

a = [1.0d0, 2.0d0, 3.0d0]
b = [0.5d0, 1.5d0, 2.5d0]
call add_arrays(n, a, b, c)

if (abs(c(1) - 1.5d0) > 1.0d-12) error stop
if (abs(c(2) - 3.5d0) > 1.0d-12) error stop
if (abs(c(3) - 5.5d0) > 1.0d-12) error stop

end program
