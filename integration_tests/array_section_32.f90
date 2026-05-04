program array_section_32
    implicit none

    real(8) :: x(3, 4, 2), y(3, 4, 2)
    integer :: i, j

    do j = 1, 4
        do i = 1, 3
            x(i, j, 1) = dble(i + 10*j)
            x(i, j, 2) = dble(100 + i + 10*j)
            y(i, j, 1) = dble(2*i + 10*j)
            y(i, j, 2) = dble(100 + 3*i + 10*j)
        end do
    end do

    x(:, :, 1) = 0.5d0*(x(:, :, 1) + x(:, :, 2))
    x(:, :, 2) = x(:, :, 1) - x(:, :, 2)

    y(:, :, 1) = y(:, :, 1) + y(:, :, 2)
    y(:, :, 2) = y(:, :, 1) - 2.0d0*y(:, :, 2)

    do j = 1, 4
        do i = 1, 3
            if (abs(x(i, j, 1) - dble(50 + i + 10*j)) > 1.0d-12) error stop 1
            if (abs(x(i, j, 2) + 50.0d0) > 1.0d-12) error stop 2
            if (abs(y(i, j, 1) - dble(100 + 5*i + 20*j)) > 1.0d-12) error stop 3
            if (abs(y(i, j, 2) + dble(100 + i)) > 1.0d-12) error stop 4
        end do
    end do
end program
