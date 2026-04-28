module arrays_127_m
implicit none
contains

subroutine bump_int(x)
integer, intent(inout) :: x(:)
x(1) = x(1) + 10
end subroutine

subroutine check_int(x)
integer, intent(in) :: x(:)
if (x(1) /= 11 .or. x(2) /= 2) error stop
end subroutine

end module

program arrays_127
use arrays_127_m, only: bump_int, check_int
implicit none
integer, allocatable :: x(:)

allocate(x(2))
x = [1, 2]
call bump_int(x)
call check_int(x)
print *, x
end program
