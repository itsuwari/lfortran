module c_backend_descriptor_vector_assignment_direct_01_m
implicit none

contains

subroutine shift_column(xyz, trans, vij, jat, iat, jtr)
real(8), intent(in) :: xyz(:, :)
real(8), intent(in) :: trans(:, :)
real(8), intent(out) :: vij(3)
integer, intent(in) :: jat, iat, jtr
real(8) :: local_vij(3)

local_vij(:) = xyz(:, jat) + trans(:, jtr) - xyz(:, iat)
vij(:) = local_vij(:)

end subroutine

end module

program c_backend_descriptor_vector_assignment_direct_01
use c_backend_descriptor_vector_assignment_direct_01_m, only: shift_column
implicit none

real(8) :: xyz(3, 2), trans(3, 1), vij(3)

xyz = reshape([1.0d0, 2.0d0, 3.0d0, 4.0d0, 5.0d0, 6.0d0], [3, 2])
trans(:, 1) = [0.5d0, -1.0d0, 2.0d0]

call shift_column(xyz, trans, vij, 2, 1, 1)

if (abs(vij(1) - 3.5d0) > 1.0d-12) error stop
if (abs(vij(2) - 2.0d0) > 1.0d-12) error stop
if (abs(vij(3) - 5.0d0) > 1.0d-12) error stop

end program
