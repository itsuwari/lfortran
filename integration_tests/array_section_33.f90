program array_section_33
    implicit none

    real(8) :: x(2, 3, 4, 2), y(2, 3, 4, 2)
    integer :: i, j, k

    do k = 1, 4
        do j = 1, 3
            do i = 1, 2
                x(i, j, k, 1) = dble(i + 10*j + 100*k)
                x(i, j, k, 2) = dble(1000 + i + 10*j + 100*k)
                y(i, j, k, 1) = dble(2*i + 10*j + 100*k)
                y(i, j, k, 2) = dble(1000 + 3*i + 10*j + 100*k)
            end do
        end do
    end do

    x(:, :, :, 1) = 0.5d0*(x(:, :, :, 1) + x(:, :, :, 2))
    x(:, :, :, 2) = x(:, :, :, 1) - x(:, :, :, 2)

    y(:, :, :, 1) = y(:, :, :, 1) + y(:, :, :, 2)
    y(:, :, :, 2) = y(:, :, :, 1) - 2.0d0*y(:, :, :, 2)

    do k = 1, 4
        do j = 1, 3
            do i = 1, 2
                if (abs(x(i, j, k, 1) - dble(500 + i + 10*j + 100*k)) > 1.0d-12) error stop 1
                if (abs(x(i, j, k, 2) + 500.0d0) > 1.0d-12) error stop 2
                if (abs(y(i, j, k, 1) - dble(1000 + 5*i + 20*j + 200*k)) > 1.0d-12) error stop 3
                if (abs(y(i, j, k, 2) + dble(1000 + i)) > 1.0d-12) error stop 4
            end do
        end do
    end do
end program
