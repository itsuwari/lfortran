program c_backend_small_integer_pow_01
implicit none

integer :: n
real :: x, y, z, w
real(8) :: d, q
real :: a(3)

x = 3.0
n = 6
d = 0.25_8
a = [1.0, 2.0, 3.0]
y = x**2 + x**3 + (x + 1.0)**2
z = next_value()**2
w = a(1)**2 + a(2)**3 + a(3)**4
q = (0.5_8 / d)**(n / 2) + d**(-2)

if (abs(y - 52.0) > 1e-6) error stop
if (abs(z - 4.0) > 1e-6) error stop
if (abs(w - 90.0) > 1e-6) error stop
if (abs(q - 24.0_8) > 1e-12_8) error stop

contains

function next_value() result(r)
real :: r

r = 2.0
end function

end program
