program c_backend_sum_rank3_induction_01
    real(8) :: a(2, 3, 4)
    integer :: i, j, k

    do k = 1, 4
        do j = 1, 3
            do i = 1, 2
                a(i, j, k) = real(i + 10*j + 100*k, 8)
            end do
        end do
    end do

    if (abs(sum(a) - 6516.0d0) > 1.0d-12) error stop
end program
