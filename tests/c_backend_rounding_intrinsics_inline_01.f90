program c_backend_rounding_intrinsics_inline_01
implicit none

real :: r4
real(8) :: r8
integer :: i

r4 = -4.75
r8 = 3.5d0

if (aint(r4) /= -4.0) error stop
if (aint(r8) /= 3.0d0) error stop

if (anint(r4) /= -5.0) error stop
if (anint(r8) /= 4.0d0) error stop

i = nint(r4)
if (i /= -5) error stop
i = nint(r8)
if (i /= 4) error stop

end program
