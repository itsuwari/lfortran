program c_backend_rank2_spread_outer_update_scalarized_01
implicit none

real(8), allocatable :: sigma(:, :), expected(:, :)
real(8) :: vec(3), dg(3)
integer :: i, j

allocate(sigma(3, 3), expected(3, 3))
sigma = 10.0_8
vec = [1.0_8, 2.0_8, 3.0_8]
dg = [4.0_8, 5.0_8, 6.0_8]
expected = sigma

do j = 1, 3
    do i = 1, 3
        expected(i, j) = expected(i, j) + 0.5_8 * &
            (vec(j) * dg(i) + dg(j) * vec(i))
    end do
end do

call update_sigma(sigma, vec, dg)

if (any(abs(sigma - expected) > 1.0e-12_8)) error stop

contains

subroutine update_sigma(sigma, vec, dg)
real(8), contiguous, intent(inout) :: sigma(:, :)
real(8), intent(in) :: vec(:), dg(:)

sigma(:, :) = sigma + 0.5_8 * (spread(vec, 1, 3) * spread(dg, 2, 3) &
    + spread(dg, 1, 3) * spread(vec, 2, 3))

end subroutine

end program
