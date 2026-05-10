subroutine bump(x)
integer :: x
x = x + 1
end subroutine

program main
integer :: a
a = 1
call bump(a)
if (a /= 2) error stop
end program
