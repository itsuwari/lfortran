program c_backend_lazy_intrinsic_temp_init_01
implicit none

real(8) :: lattice(3, 3)
real(8) :: alpha

lattice = reshape([ &
    1.0_8, 2.0_8, 3.0_8, &
    4.0_8, 5.0_8, 6.0_8, &
    7.0_8, 8.0_8, 9.0_8], [3, 3])

call combine(lattice, alpha)

if (abs(alpha - (sqrt(14.0_8) + sqrt(66.0_8))) > 1.0e-12_8) error stop

contains

subroutine combine(lattice, alpha)
real(8), intent(in) :: lattice(:, :)
real(8), intent(out) :: alpha

alpha = sqrt(minval(sum(lattice(:, :) * lattice(:, :), 1))) &
    + sqrt(minval(sum(lattice(:, :) * lattice(:, :), 2)))
end subroutine combine

end program c_backend_lazy_intrinsic_temp_init_01
