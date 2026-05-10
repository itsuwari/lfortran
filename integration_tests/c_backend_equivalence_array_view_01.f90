program c_backend_equivalence_array_view_01
implicit none

real(8) :: a(2, 2)
real(8) :: av(4)

equivalence (a(1, 1), av(1))

a(1, 1) = 11.0d0
a(2, 1) = 22.0d0
a(1, 2) = 33.0d0
a(2, 2) = 44.0d0

if (av(1) /= 11.0d0) error stop
if (av(2) /= 22.0d0) error stop
if (av(3) /= 33.0d0) error stop
if (av(4) /= 44.0d0) error stop

av(1) = 55.0d0
if (a(1, 1) /= 55.0d0) error stop

end program
