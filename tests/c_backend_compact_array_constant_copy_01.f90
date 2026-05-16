program c_backend_compact_array_constant_copy_01
implicit none

real(8), parameter :: table(10) = [ &
    1.0d0, 2.0d0, 3.0d0, 4.0d0, 5.0d0, &
    6.0d0, 7.0d0, 8.0d0, 9.0d0, 10.0d0]
real(8), allocatable :: x(:)
integer :: i

allocate(x(12))
x = 0.0d0
x(2:11) = [ &
    1.0d0, 2.0d0, 3.0d0, 4.0d0, 5.0d0, &
    6.0d0, 7.0d0, 8.0d0, 9.0d0, 10.0d0]

do i = 1, 10
    if (x(i + 1) /= table(i)) error stop
end do
if (x(1) /= 0.0d0 .or. x(12) /= 0.0d0) error stop
end program c_backend_compact_array_constant_copy_01
