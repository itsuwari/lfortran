subroutine bump_first(x)
double precision :: x(*)
x(1) = x(1) + 1.0d0
end subroutine

subroutine caller_rank2(a)
double precision :: a(2, 2)
external bump_first
call bump_first(a)
end subroutine

subroutine caller_rank1(a)
double precision :: a(4)
external bump_first
call bump_first(a)
end subroutine

program main
double precision :: a(2, 2), b(4)
a(1, 1) = 1.0d0
b(1) = 2.0d0
call caller_rank2(a)
call caller_rank1(b)
if (a(1, 1) /= 2.0d0) error stop
if (b(1) /= 3.0d0) error stop
end program
