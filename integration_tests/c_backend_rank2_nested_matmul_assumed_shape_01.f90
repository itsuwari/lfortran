module c_backend_rank2_nested_matmul_assumed_shape_01_m
implicit none

real(8), parameter :: left(2, 3) = reshape([&
    1.0_8, 2.0_8, &
    3.0_8, 4.0_8, &
    5.0_8, 6.0_8], shape(left))
real(8), parameter :: right(4, 3) = reshape([&
    11.0_8, 12.0_8, 13.0_8, 14.0_8, &
    21.0_8, 22.0_8, 23.0_8, 24.0_8, &
    31.0_8, 32.0_8, 33.0_8, 34.0_8], shape(right))

contains

subroutine transform(cart, sphr)
real(8), intent(in) :: cart(:, :)
real(8), intent(out) :: sphr(:, :)

sphr = matmul(left, matmul(cart, transpose(right)))
end subroutine

end module

program c_backend_rank2_nested_matmul_assumed_shape_01
use c_backend_rank2_nested_matmul_assumed_shape_01_m, only: left, right, transform
implicit none

real(8) :: cart(3, 3), sphr(2, 4), expected(2, 4), inner(3, 4)
integer :: i, j, k

do j = 1, 3
    do i = 1, 3
        cart(i, j) = real(100 * i + j, 8)
    end do
end do

inner = 0.0_8
do j = 1, 4
    do i = 1, 3
        do k = 1, 3
            inner(i, j) = inner(i, j) + cart(i, k) * right(j, k)
        end do
    end do
end do

expected = 0.0_8
do j = 1, 4
    do i = 1, 2
        do k = 1, 3
            expected(i, j) = expected(i, j) + left(i, k) * inner(k, j)
        end do
    end do
end do

call transform(cart, sphr)

do j = 1, 4
    do i = 1, 2
        if (abs(sphr(i, j) - expected(i, j)) > 1.0e-12_8) error stop
    end do
end do

end program
