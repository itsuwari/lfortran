module arrays_129_m
implicit none

type cgto_type
    integer :: ang
end type

integer, parameter :: msao(0:2) = [1, 3, 5]

contains

subroutine fill_rank3(a, n, m)
    integer, intent(in) :: n, m
    real(8), intent(out) :: a(3, n, m)

    a(2, 3, 4) = 234.0_8
end subroutine

subroutine fill_rank3_struct_dim(cgto, a, m)
    type(cgto_type), intent(in) :: cgto
    integer, intent(in) :: m
    real(8), intent(out) :: a(3, msao(cgto%ang), m)

    a(2, 3, 4) = 345.0_8
end subroutine

end module

program arrays_129
use arrays_129_m, only: cgto_type, fill_rank3, fill_rank3_struct_dim
implicit none

type(cgto_type) :: cgto
real(8) :: x(3, 20)

x = -1.0_8
call fill_rank3(x, 5, 4)
print *, x(2, 18)
if (x(2, 18) /= 234.0_8) error stop

cgto%ang = 1
x = -1.0_8
call fill_rank3_struct_dim(cgto, x, 4)
print *, x(2, 12)
if (x(2, 12) /= 345.0_8) error stop

end program
