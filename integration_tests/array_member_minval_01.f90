module array_member_minval_01_m
implicit none
integer, parameter :: wp = kind(1.0d0), maxg = 6

type :: cgto_type
    integer :: nprim = 0
    real(wp) :: alpha(maxg) = 0.0_wp
end type

contains

subroutine test(cgto, n, out)
    type(cgto_type), intent(in) :: cgto(:)
    integer, intent(in) :: n
    real(wp), intent(out) :: out
    integer :: i

    out = huge(out)
    do i = 1, n
        out = min(out, minval(cgto(i)%alpha(:cgto(i)%nprim)))
    end do
end subroutine

end module

program main
use array_member_minval_01_m
implicit none
type(cgto_type) :: a(1)
real(wp) :: out

a(1)%nprim = 3
a(1)%alpha(1:3) = [3.0_wp, 2.0_wp, 4.0_wp]
call test(a, 1, out)
if (abs(out - 2.0_wp) > 1e-12_wp) error stop
end program
