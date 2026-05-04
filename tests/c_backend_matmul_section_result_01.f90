program c_backend_matmul_section_result_01
implicit none
real :: lattice(3, 3), coord(3)
real, allocatable :: xyz(:, :)

allocate(xyz(3, 2))
lattice = reshape([1.0, 2.0, 3.0, &
                   4.0, 5.0, 6.0, &
                   7.0, 8.0, 9.0], [3, 3])
coord = [2.0, 3.0, 4.0]
xyz = 0.0

xyz(:, 2) = matmul(lattice, coord)

if (abs(xyz(1, 2) - 42.0) > 1.e-5) error stop
if (abs(xyz(2, 2) - 51.0) > 1.e-5) error stop
if (abs(xyz(3, 2) - 60.0) > 1.e-5) error stop
if (abs(xyz(1, 1)) > 1.e-5) error stop
if (abs(xyz(2, 1)) > 1.e-5) error stop
if (abs(xyz(3, 1)) > 1.e-5) error stop
end program
