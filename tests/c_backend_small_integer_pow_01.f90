program c_backend_small_integer_pow_01
implicit none

real :: x, y, z

x = 3.0
y = x**2 + x**3 + (x + 1.0)**2
z = next_value()**2

if (abs(y - 52.0) > 1e-6) error stop
if (abs(z - 4.0) > 1e-6) error stop

contains

function next_value() result(r)
real :: r

r = 2.0
end function

end program
