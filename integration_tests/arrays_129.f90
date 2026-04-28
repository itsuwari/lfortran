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

subroutine fill_rank2(a, n, m)
    integer, intent(in) :: n, m
    real(8), intent(out) :: a(n, m)

    a(:, :) = 0.0_8
    a(2, 3) = 456.0_8
end subroutine

subroutine pass_rank1_to_rank2(x)
    real(8), intent(out) :: x(:)

    call fill_rank2(x, 4, 3)
end subroutine

subroutine read_lower_bound_zero(a, value)
    real(8), intent(in) :: a(0:)
    real(8), intent(out) :: value

    value = a(0) + 10.0_8*a(1) + 100.0_8*a(2)
end subroutine

subroutine pass_lower_bound_zero(value)
    real(8), intent(out) :: value
    real(8) :: local(0:2)

    local(0) = 7.0_8
    local(1) = 11.0_8
    local(2) = 13.0_8
    call read_lower_bound_zero(local, value)
end subroutine

end module

program arrays_129
use arrays_129_m, only: cgto_type, fill_rank3, fill_rank3_struct_dim, &
    pass_rank1_to_rank2, pass_lower_bound_zero
implicit none

type(cgto_type) :: cgto
real(8) :: x(3, 20)
real(8) :: y(12)
real(8) :: z

x = -1.0_8
call fill_rank3(x, 5, 4)
print *, x(2, 18)
if (x(2, 18) /= 234.0_8) error stop

cgto%ang = 1
x = -1.0_8
call fill_rank3_struct_dim(cgto, x, 4)
print *, x(2, 12)
if (x(2, 12) /= 345.0_8) error stop

y = -1.0_8
call pass_rank1_to_rank2(y)
print *, y(10)
if (y(10) /= 456.0_8) error stop

call pass_lower_bound_zero(z)
print *, z
if (z /= 1417.0_8) error stop

end program
