module c_backend_specialized_pass_array_no_copy_01_a
implicit none

type :: container
    real(8) :: bias
end type

contains

subroutine inner(mol, xyz, trans, total)
type(container), intent(in) :: mol
real(8), intent(in) :: xyz(:, :)
real(8), intent(in) :: trans(:, :)
real(8), intent(inout) :: total

total = total + mol%bias + xyz(2, 1) + trans(3, 1)

end subroutine

end module

module c_backend_specialized_pass_array_no_copy_01_b
use c_backend_specialized_pass_array_no_copy_01_a, only: container, inner
implicit none

contains

subroutine outer(mol, xyz, trans, total)
type(container), intent(in) :: mol
real(8), intent(in) :: xyz(:, :)
real(8), intent(in) :: trans(:, :)
real(8), intent(inout) :: total

call inner(mol, xyz, trans, total)

end subroutine

end module

program c_backend_specialized_pass_array_no_copy_01
use c_backend_specialized_pass_array_no_copy_01_a, only: container
use c_backend_specialized_pass_array_no_copy_01_b, only: outer
implicit none

type(container) :: mol
real(8) :: xyz(3, 2), trans(3, 1), total

mol%bias = 1.0d0
xyz = reshape([10.0d0, 20.0d0, 30.0d0, 40.0d0, 50.0d0, 60.0d0], [3, 2])
trans(:, 1) = [100.0d0, 200.0d0, 300.0d0]
total = 0.0d0

call outer(mol, xyz, trans, total)

if (abs(total - 321.0d0) > 1.0d-12) error stop

end program
