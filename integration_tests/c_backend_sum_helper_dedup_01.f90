module c_backend_sum_helper_dedup_01_m
implicit none

contains

subroutine sum_assumed(x, s)
    real(8), intent(in) :: x(:, :)
    real(8), intent(out) :: s

    s = sum(x)
end subroutine sum_assumed

end module c_backend_sum_helper_dedup_01_m

program c_backend_sum_helper_dedup_01
use c_backend_sum_helper_dedup_01_m
implicit none

real(8) :: a(4, 5), s

a = 1.0d0
call sum_assumed(a(:, 2:4), s)

if (abs(s - 12.0d0) > 1.0d-12) error stop
if (abs(sum(a(:, 1:1)) - 4.0d0) > 1.0d-12) error stop
if (abs(sum(a(:, 5:5)) - 4.0d0) > 1.0d-12) error stop

end program c_backend_sum_helper_dedup_01
