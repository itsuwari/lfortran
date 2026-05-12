program c_backend_product_rank3_induction_01
    integer(8) :: a(2, 2, 2)
    integer :: i, j, k, n

    n = 0
    do k = 1, 2
        do j = 1, 2
            do i = 1, 2
                n = n + 1
                a(i, j, k) = int(n, 8)
            end do
        end do
    end do

    if (product(a) /= 40320_8) error stop
end program
