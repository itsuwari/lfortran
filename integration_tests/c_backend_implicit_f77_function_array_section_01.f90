program c_backend_implicit_f77_function_array_section_01
implicit none

double precision :: a(3, 3), got, expected
double precision :: ddot_local
external ddot_local
integer :: i, j

do j = 1, 3
    do i = 1, 3
        a(i, j) = 10.0d0*j + i
    end do
end do

got = ddot_local(2, a(1, 2), 1, a(1, 3), 1)
expected = a(1, 2)*a(1, 3) + a(2, 2)*a(2, 3)
if (abs(got - expected) > 1.0d-12) error stop

end program

double precision function ddot_local(n, x, incx, y, incy)
implicit none

integer :: n, incx, incy, i
double precision :: x(*), y(*)

ddot_local = 0.0d0
do i = 1, n
    ddot_local = ddot_local + x(1 + (i - 1)*incx)*y(1 + (i - 1)*incy)
end do

end function
