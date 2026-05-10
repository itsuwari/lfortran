program c_backend_real_bitcast_exponent_01
implicit none

double precision :: x
integer :: e

x = 8.0d0
e = exponent(x)
if (e /= 4) error stop

x = 0.0d0
e = exponent(x)
if (e /= 0) error stop
end program
