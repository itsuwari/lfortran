module c_backend_rank2_matmul_assumed_shape_scalarized_01_m
implicit none

contains

subroutine multiply(a, b, c)
real(8), intent(in) :: a(:, :), b(:, :)
real(8), intent(out) :: c(:, :)

c = matmul(a, b)
end subroutine

end module

program c_backend_rank2_matmul_assumed_shape_scalarized_01
use c_backend_rank2_matmul_assumed_shape_scalarized_01_m, only: multiply
implicit none

real(8) :: a(2, 3), b(3, 4), c(2, 4), expected(2, 4)
integer :: i, j, k

do j = 1, 3
    do i = 1, 2
        a(i, j) = real(10 * i + j, 8)
    end do
end do

do j = 1, 4
    do i = 1, 3
        b(i, j) = real(100 * i + j, 8)
    end do
end do

expected = 0.0_8
do j = 1, 4
    do i = 1, 2
        do k = 1, 3
            expected(i, j) = expected(i, j) + a(i, k) * b(k, j)
        end do
    end do
end do

call multiply(a, b, c)

do j = 1, 4
    do i = 1, 2
        if (abs(c(i, j) - expected(i, j)) > 1.0e-12_8) error stop
    end do
end do

end program
