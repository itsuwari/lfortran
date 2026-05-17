program c_backend_rank2_transpose_scalarized_01
implicit none

real(8), allocatable :: sigma(:, :), expected(:, :)
integer :: i, j

allocate(sigma(2, 2), expected(2, 2))
sigma = 10.0_8
expected = sigma

do j = 1, 2
    do i = 1, 2
        expected(i, j) = expected(i, j) - 0.5_8 * (matrix_value(i, j) + matrix_value(j, i))
    end do
end do

call update_sigma(sigma)

if (any(abs(sigma - expected) > 1.0e-12_8)) error stop

contains

pure real(8) function matrix_value(i, j)
integer, intent(in) :: i, j
matrix_value = real(10 * j + i, 8)
end function

subroutine update_sigma(sigma)
real(8), contiguous, intent(inout) :: sigma(:, :)
real(8), allocatable :: stmp(:, :)
integer :: i, j

allocate(stmp(size(sigma, 1), size(sigma, 2)))

do j = 1, size(stmp, 2)
    do i = 1, size(stmp, 1)
        stmp(i, j) = matrix_value(i, j)
    end do
end do

sigma(:, :) = sigma - 0.5_8 * (stmp + transpose(stmp))

end subroutine

end program
