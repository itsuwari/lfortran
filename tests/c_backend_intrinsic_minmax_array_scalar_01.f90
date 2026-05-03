program c_backend_intrinsic_minmax_array_scalar_01
implicit none

real(8) :: a(3), b(3), c(3)
real(8) :: lat(3, 3), vec(3)
integer :: ia(3), ib(3), ic(3)

b = [-1.0d0, 2.0d0, -3.0d0]
a(:) = max(b(:), 0.0d0)
c(:) = min(0.0d0, b(:))

if (abs(a(1)) > 1d-12) error stop
if (abs(a(2) - 2.0d0) > 1d-12) error stop
if (abs(a(3)) > 1d-12) error stop
if (abs(c(1) + 1.0d0) > 1d-12) error stop
if (abs(c(2)) > 1d-12) error stop
if (abs(c(3) + 3.0d0) > 1d-12) error stop

ib = [-1, 2, -3]
ia(:) = max(ib(:), 0)
ic(:) = min(0, ib(:))

if (any(ia /= [0, 2, 0])) error stop
if (any(ic /= [-1, 0, -3])) error stop

lat = reshape([1.0d0, 2.0d0, 2.0d0, 2.0d0, 3.0d0, 6.0d0, &
    4.0d0, 4.0d0, 7.0d0], [3, 3])
vec = sqrt(sum(lat**2, 1))

if (abs(vec(1) - 3.0d0) > 1d-12) error stop
if (abs(vec(2) - 7.0d0) > 1d-12) error stop
if (abs(vec(3) - 9.0d0) > 1d-12) error stop

end program
