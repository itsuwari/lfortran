module arrays_126_m
implicit none
contains

subroutine bump(x)
real(8), intent(inout) :: x(3)
x = x + 10.0d0
end subroutine

end module

program arrays_126
use arrays_126_m, only: bump
implicit none
real(8) :: x(6)
integer :: i

x = [(real(i, 8), i = 1, 6)]
call bump(x(1:6:2))

if (any(abs(x - [11.0d0, 2.0d0, 13.0d0, 4.0d0, 15.0d0, 6.0d0]) > 1.0d-12)) then
    error stop
end if
print *, x
end program
