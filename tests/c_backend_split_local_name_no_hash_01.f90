program c_backend_split_local_name_no_hash_01
implicit none

real(8) :: x(2, 4), w(4)

x = 0.0d0
w = 0.0d0
call fill(x(:, :), w(:))

if (abs(x(1, 2) - 3.0d0) > 1.0d-12) error stop
if (abs(x(2, 2) - 4.0d0) > 1.0d-12) error stop
if (abs(w(2) - 5.0d0) > 1.0d-12) error stop

contains

subroutine fill(a, b)
real(8), intent(inout) :: a(:, :)
real(8), intent(inout) :: b(:)

call gen(a(:, 2), b(2), 3.0d0)

end subroutine

subroutine gen(a, b, v)
real(8), intent(inout) :: a(2)
real(8), intent(inout) :: b
real(8), intent(in) :: v

a(1) = v
a(2) = v + 1.0d0
b = v + 2.0d0

end subroutine

end program
