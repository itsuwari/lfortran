module arrays_128_m
implicit none
contains

subroutine set_first_real(x)
real(8), intent(inout) :: x(:)
x(1) = 42.0d0
end subroutine

subroutine set_first_two_int(x)
integer, intent(inout) :: x(:)
x(1) = 9
x(2) = 8
end subroutine

end module

program arrays_128
use arrays_128_m, only: set_first_real, set_first_two_int
implicit none
real(8), allocatable :: x(:)
integer :: y(0:5)

allocate(x(0:2))
x = 1.0d0
call set_first_real(x)
if (x(0) /= 42.0d0 .or. x(1) /= 1.0d0) error stop

y = 1
call set_first_two_int(y(0:4:2))
if (y(0) /= 9 .or. y(1) /= 1 .or. y(2) /= 8 .or. y(4) /= 1) error stop

print *, x(0), x(1), y(0), y(1), y(2), y(4)
end program
