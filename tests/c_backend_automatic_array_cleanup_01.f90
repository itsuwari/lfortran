program c_backend_automatic_array_cleanup_01
implicit none

real(8) :: value

call accumulate(6, value)
if (abs(value - 63.0_8) > 1.0e-12_8) error stop

call accumulate(0, value)
if (value /= -1.0_8) error stop

contains

subroutine accumulate(n, total)
    integer, intent(in) :: n
    real(8), intent(out) :: total
    real(8) :: work(n, 3)
    integer :: i

    total = -1.0_8
    if (n <= 0) return

    do i = 1, n
        work(i, :) = real(i, 8)
    end do
    total = sum(work)
end subroutine

end program
