program c_backend_spread_allocatable_assignment_01
implicit none

real(8), allocatable :: a(:, :)
real(8) :: v(3)

v = [1.0d0, 2.0d0, 3.0d0]
a = spread(v, 1, 1)

if (size(a, 1) /= 1) error stop
if (size(a, 2) /= 3) error stop
if (abs(sum(a) - 6.0d0) > 1.0d-12) error stop
end program c_backend_spread_allocatable_assignment_01
