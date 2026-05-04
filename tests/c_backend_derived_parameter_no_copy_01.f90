module c_backend_derived_parameter_no_copy_01_a
implicit none

type :: cutoff_t
    real(8) :: scale
end type

contains

subroutine inner(cutoff_arg, energy2, energy3)
type(cutoff_t), intent(in) :: cutoff_arg
real(8), intent(out) :: energy2(:, :)
real(8), intent(out) :: energy3(:, :)

energy2 = cutoff_arg%scale
energy3 = 2.0d0*cutoff_arg%scale

end subroutine

end module

module c_backend_derived_parameter_no_copy_01_b
use c_backend_derived_parameter_no_copy_01_a, only: cutoff_t, inner
implicit none

type(cutoff_t), parameter :: cutoff = cutoff_t(3.0d0)

contains

subroutine outer(total)
real(8), intent(inout) :: total

real(8), allocatable :: energy2(:, :), energy3(:, :)

allocate(energy2(2, 2), energy3(2, 2))
call inner(cutoff, energy2, energy3)
total = sum(energy2) + sum(energy3)

end subroutine

end module

program c_backend_derived_parameter_no_copy_01
use c_backend_derived_parameter_no_copy_01_b, only: outer
implicit none

real(8) :: total

total = 0.0d0

call outer(total)

if (abs(total - 36.0d0) > 1.0d-12) error stop

end program
