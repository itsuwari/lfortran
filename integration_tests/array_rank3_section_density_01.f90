module array_rank3_section_density_01_m
implicit none

type :: solver_type
contains
    procedure :: get_density => solver_get_density
end type

contains

subroutine gemm_t(amat, bmat, cmat, transb)
    real(8), intent(in) :: amat(:, :)
    real(8), intent(in) :: bmat(:, :)
    real(8), intent(inout) :: cmat(:, :)
    character(len=1), intent(in), optional :: transb
    integer :: i, j, k

    cmat = 0.0d0
    if (present(transb)) then
        if (transb == 't' .or. transb == 'T') then
            do j = 1, size(cmat, 2)
                do i = 1, size(cmat, 1)
                    do k = 1, size(amat, 2)
                        cmat(i, j) = cmat(i, j) + amat(i, k)*bmat(j, k)
                    end do
                end do
            end do
            return
        end if
    end if

    do j = 1, size(cmat, 2)
        do i = 1, size(cmat, 1)
            do k = 1, size(amat, 2)
                cmat(i, j) = cmat(i, j) + amat(i, k)*bmat(k, j)
            end do
        end do
    end do
end subroutine

subroutine get_density_matrix(focc, coeff, pmat)
    real(8), intent(in) :: focc(:)
    real(8), contiguous, intent(in) :: coeff(:, :)
    real(8), contiguous, intent(out) :: pmat(:, :)
    real(8), allocatable :: scratch(:, :)
    integer :: iao, jao

    allocate(scratch(size(pmat, 1), size(pmat, 2)))
    do iao = 1, size(pmat, 1)
        do jao = 1, size(pmat, 2)
            scratch(jao, iao) = coeff(jao, iao)*focc(iao)
        end do
    end do
    call gemm_t(scratch, coeff, pmat, transb='t')
end subroutine

subroutine solver_get_density(self, hmat, focc, density)
    class(solver_type), intent(inout) :: self
    real(8), contiguous, intent(in) :: hmat(:, :, :)
    real(8), contiguous, intent(in) :: focc(:, :)
    real(8), contiguous, intent(inout) :: density(:, :, :)
    integer :: spin

    do spin = 1, size(density, 3)
        call get_density_matrix(focc(:, spin), hmat(:, :, spin), density(:, :, spin))
    end do
end subroutine

subroutine reference_density(focc, coeff, pmat)
    real(8), intent(in) :: focc(:)
    real(8), intent(in) :: coeff(:, :)
    real(8), intent(out) :: pmat(:, :)
    integer :: i, j, k

    pmat = 0.0d0
    do j = 1, size(pmat, 2)
        do i = 1, size(pmat, 1)
            do k = 1, size(coeff, 2)
                pmat(i, j) = pmat(i, j) + coeff(i, k)*focc(k)*coeff(j, k)
            end do
        end do
    end do
end subroutine

end module

program array_rank3_section_density_01
use array_rank3_section_density_01_m
implicit none

type(solver_type) :: solver
real(8) :: coeff(0:2, -1:1, 2:3)
real(8) :: focc(-1:1, 2:3)
real(8) :: density(0:2, -1:1, 2:3)
real(8), allocatable :: coeff_alloc(:, :, :)
real(8), allocatable :: focc_alloc(:, :)
real(8), allocatable :: density_alloc(:, :, :)
real(8) :: expected(3, 3)
integer :: i, j, spin

allocate(coeff_alloc(0:2, -1:1, 2:3))
allocate(focc_alloc(-1:1, 2:3))
allocate(density_alloc(0:2, -1:1, 2:3))

do spin = 2, 3
    do j = -1, 1
        focc(j, spin) = 0.25d0*dble(j + 3) + 0.125d0*dble(spin)
        focc_alloc(j, spin) = focc(j, spin)
        do i = 0, 2
            coeff(i, j, spin) = 0.5d0*dble(spin) + 0.125d0*dble(i + 1) &
                + 0.0625d0*dble(j + 2) + 0.01d0*dble((i + 1)*(j + 3))
            density(i, j, spin) = -999.0d0
            coeff_alloc(i, j, spin) = coeff(i, j, spin)
            density_alloc(i, j, spin) = -999.0d0
        end do
    end do
end do

call solver%get_density(coeff, focc, density)
call solver%get_density(coeff_alloc, focc_alloc, density_alloc)

do spin = 2, 3
    call reference_density(focc(:, spin), coeff(:, :, spin), expected)
    do j = 1, 3
        do i = 1, 3
            if (abs(density(i - 1, j - 2, spin) - expected(i, j)) > 1.0d-12) then
                error stop
            end if
            if (abs(density_alloc(i - 1, j - 2, spin) - expected(i, j)) > 1.0d-12) then
                error stop
            end if
        end do
    end do
end do

print *, density(0, -1, 2), density(2, 1, 3)
end program
