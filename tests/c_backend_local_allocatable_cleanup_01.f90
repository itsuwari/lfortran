program c_backend_local_allocatable_cleanup_01
implicit none

real(8) :: total

call fill(4, total)
if (abs(total - 20.0_8) > 1.0e-12_8) error stop

call fill(0, total)
if (total /= -1.0_8) error stop

call fill_then_return(5, total)
if (abs(total - 5.0_8) > 1.0e-12_8) error stop

contains

subroutine fill(n, total)
    integer, intent(in) :: n
    real(8), intent(out) :: total
    real(8), allocatable :: scratch(:, :)
    integer :: i

    total = -1.0_8
    if (n <= 0) return

    allocate(scratch(n, 2))
    do i = 1, n
        scratch(i, :) = real(i, 8)
    end do
    total = sum(scratch)
end subroutine

subroutine fill_then_return(n, total)
    integer, intent(in) :: n
    real(8), intent(out) :: total
    real(8), allocatable :: scratch(:)

    allocate(scratch(n))
    scratch = 1.0_8
    total = sum(scratch)
    if (total > 0.0_8) return
    error stop
end subroutine

end program
