module c_backend_rank2_dynamic_spread_field_01_mod
implicit none

type :: field_t
    real(8) :: efield(3)
end type

type :: molecule_t
    integer :: nat
    real(8), allocatable :: xyz(:, :)
end type

type :: potential_t
    real(8), allocatable :: vat(:, :)
    real(8), allocatable :: vdp(:, :, :)
end type

contains

subroutine apply_potential(self, mol, pot)
    type(field_t), intent(in) :: self
    type(molecule_t), intent(in) :: mol
    type(potential_t), intent(inout) :: pot

    pot%vat(:, 1) = pot%vat(:, 1) - matmul(self%efield, mol%xyz)
    pot%vdp(:, :, 1) = pot%vdp(:, :, 1) - spread(self%efield, 2, mol%nat)
end subroutine

subroutine apply_field(self, mol, gradient, sigma)
    type(field_t), intent(in) :: self
    type(molecule_t), intent(in) :: mol
    real(8), intent(inout) :: gradient(:, :)
    real(8), intent(inout) :: sigma(:, :)
    real(8), allocatable :: vdp(:, :), stmp(:, :)

    vdp = spread(self%efield, 2, mol%nat)
    stmp = matmul(vdp, transpose(mol%xyz))
    gradient(:, :) = gradient - vdp
    sigma(:, :) = sigma - 0.5_8 * (stmp + transpose(stmp))
end subroutine

end module

program c_backend_rank2_dynamic_spread_field_01
use c_backend_rank2_dynamic_spread_field_01_mod, only: field_t, molecule_t, potential_t, &
    apply_potential, apply_field
implicit none

type(field_t) :: field
type(molecule_t) :: mol
type(potential_t) :: pot
real(8) :: gradient(3, 4), sigma(3, 3)
real(8) :: expected_gradient(3, 4), expected_sigma(3, 3)
real(8) :: expected_vat(4, 1), expected_vdp(3, 4, 1)
real(8) :: vdp(3, 4), stmp(3, 3)
integer :: i, j

field%efield = [1.0_8, -2.0_8, 0.5_8]
mol%nat = 4
allocate(mol%xyz(3, mol%nat))
mol%xyz(:, 1) = [0.1_8, 0.2_8, 0.3_8]
mol%xyz(:, 2) = [1.0_8, 1.5_8, 2.0_8]
mol%xyz(:, 3) = [-0.7_8, 0.4_8, 0.9_8]
mol%xyz(:, 4) = [2.0_8, -1.0_8, 0.6_8]
allocate(pot%vat(mol%nat, 1))
allocate(pot%vdp(3, mol%nat, 1))

do i = 1, mol%nat
    pot%vat(i, 1) = 0.125_8 * real(i, 8)
end do
do j = 1, mol%nat
    do i = 1, 3
        pot%vdp(i, j, 1) = 0.0625_8 * real(i + 3 * (j - 1), 8)
    end do
end do
gradient = reshape([(0.25_8 * real(i, 8), i = 1, 12)], shape(gradient))
sigma = reshape([(0.1_8 * real(i, 8), i = 1, 9)], shape(sigma))
do i = 1, mol%nat
    expected_vat(i, 1) = pot%vat(i, 1)
end do
do j = 1, mol%nat
    do i = 1, 3
        expected_vdp(i, j, 1) = pot%vdp(i, j, 1)
    end do
end do
expected_gradient = gradient
expected_sigma = sigma

vdp = spread(field%efield, 2, mol%nat)
stmp = matmul(vdp, transpose(mol%xyz))
expected_vat(:, 1) = expected_vat(:, 1) - matmul(field%efield, mol%xyz)
expected_vdp(:, :, 1) = expected_vdp(:, :, 1) - vdp
expected_gradient = expected_gradient - vdp
expected_sigma = expected_sigma - 0.5_8 * (stmp + transpose(stmp))

call apply_potential(field, mol, pot)
call apply_field(field, mol, gradient, sigma)

if (any(abs(pot%vat - expected_vat) > 1.0e-12_8)) error stop
if (any(abs(pot%vdp - expected_vdp) > 1.0e-12_8)) error stop
if (any(abs(gradient - expected_gradient) > 1.0e-12_8)) error stop
if (any(abs(sigma - expected_sigma) > 1.0e-12_8)) error stop

end program
