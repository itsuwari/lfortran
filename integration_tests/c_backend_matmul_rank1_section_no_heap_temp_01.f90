program c_backend_matmul_rank1_section_no_heap_temp_01
implicit none

integer :: i, j, k
real(8), allocatable :: a(:, :, :), v(:, :), y(:), expected(:)

allocate(a(3, 3, 4), v(3, 4), y(3), expected(3))

do k = 1, 4
    do j = 1, 3
        v(j, k) = real(j - k, 8) * 0.25_8
        do i = 1, 3
            a(i, j, k) = real(i + 2*j + 3*k, 8) * 0.5_8
        end do
    end do
end do

y(:) = 0.0_8
expected(:) = 0.0_8
do k = 1, 4
    y(:) = y(:) + 1.5_8 * matmul(a(:, :, k), v(:, k))
    do i = 1, 3
        do j = 1, 3
            expected(i) = expected(i) + 1.5_8 * a(i, j, k) * v(j, k)
        end do
    end do
end do

do i = 1, 3
    if (abs(y(i) - expected(i)) > 1.0e-12_8) error stop
end do

end program
