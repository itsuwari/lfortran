program c_backend_rank2_matmul_transpose_scalarized_01
implicit none

real(8), allocatable :: a(:, :), b(:, :), c(:, :), expected(:, :)
integer :: i, j, k

allocate(a(2, 3), b(4, 3), expected(2, 4))

do j = 1, size(a, 2)
    do i = 1, size(a, 1)
        a(i, j) = real(10 * i + j, 8)
    end do
end do

do j = 1, size(b, 2)
    do i = 1, size(b, 1)
        b(i, j) = real(100 * i + j, 8)
    end do
end do

expected = 0.0_8
do j = 1, size(expected, 2)
    do i = 1, size(expected, 1)
        do k = 1, size(a, 2)
            expected(i, j) = expected(i, j) + a(i, k) * b(j, k)
        end do
    end do
end do

c = matmul(a, transpose(b))

if (any(abs(c - expected) > 1.0e-12_8)) error stop

end program
