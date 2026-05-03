program c_backend_small_vector_assignment_setup_01
implicit none

real(8) :: xyz(3, 2), trans(3, 1), vij(3)

xyz = reshape([1.0d0, 2.0d0, 3.0d0, 4.0d0, 5.0d0, 6.0d0], [3, 2])
trans(:, 1) = [0.5d0, -1.0d0, 2.0d0]

vij = xyz(:, 2) + trans(:, 1) - xyz(:, 1)

if (abs(vij(1) - 3.5d0) > 1.0d-12) error stop
if (abs(vij(2) - 2.0d0) > 1.0d-12) error stop
if (abs(vij(3) - 5.0d0) > 1.0d-12) error stop

end program
