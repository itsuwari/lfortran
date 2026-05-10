module c_backend_sparse_array_constant_01_mod
implicit none

real(8), parameter :: real_values(20) = [ &
    0.0d0, 2.5d0, 0.0d0, 0.0d0, 0.0d0, &
    -3.75d0, 0.0d0, 0.0d0, 0.0d0, 0.0d0, &
    0.0d0, 0.0d0, 9.125d0, 0.0d0, 0.0d0, &
    0.0d0, 0.0d0, -0.0d0, 0.0d0, 0.0d0]

integer, parameter :: int_values(20) = [ &
    0, 0, 0, 17, 0, 0, 0, 0, 0, 0, &
    -4, 0, 0, 0, 0, 0, 0, 0, 23, 0]

contains

subroutine check_sparse_constants()
    real(8) :: neg_zero
    if (real_values(2) /= 2.5d0) error stop
    if (real_values(6) /= -3.75d0) error stop
    if (real_values(13) /= 9.125d0) error stop
    neg_zero = real_values(18)
    if (.not. (1.0d0 / neg_zero < 0.0d0)) error stop
    if (sum(abs(real_values)) /= 15.375d0) error stop
    if (int_values(4) /= 17) error stop
    if (int_values(11) /= -4) error stop
    if (int_values(19) /= 23) error stop
    if (sum(int_values) /= 36) error stop
end subroutine

end module

program c_backend_sparse_array_constant_01
use c_backend_sparse_array_constant_01_mod, only: check_sparse_constants
implicit none
call check_sparse_constants()
end program
