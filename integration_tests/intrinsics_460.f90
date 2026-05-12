program intrinsics_460
implicit none

integer :: lb(3), ub(3)
real(8) :: res

lb = 1
ub = 10
call test_proc(lb, ub, res)
if (abs(res - 1.0d0) > 1d-12) error stop

contains

    subroutine test_proc(lb, ub, res)
        integer, intent(in) :: lb(:), ub(:)
        real(8), intent(out) :: res
        integer :: i
        real(8) :: work(lb(1):ub(1))

        do i = lb(1), ub(1)
            work(i) = 1.0d0
        end do
        res = minval(work)
    end subroutine

end program
