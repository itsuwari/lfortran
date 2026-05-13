program c_backend_complex_to_integer_01
implicit none

complex :: work4(2)
complex(8) :: work8
integer :: i4
integer(8) :: i8

work4(1) = (12.75, -3.5)
work4(2) = (-2.25, 100.0)
work8 = (42.875_8, -9.0_8)

i4 = int(work4(1))
if (i4 /= 12) error stop

i4 = int(work4(2))
if (i4 /= -2) error stop

i8 = int(work8, kind=8)
if (i8 /= 42_8) error stop

end program c_backend_complex_to_integer_01
