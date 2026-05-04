program array_section_34
    implicit none

    real(8) :: x(5, 2), y(5, 2)
    integer :: i

    do i = 1, 5
        x(i, 1) = dble(i)
        x(i, 2) = dble(100 + i)
        y(i, 1) = dble(2*i)
        y(i, 2) = dble(100 + 3*i)
    end do

    x(:, 1) = 0.5d0*(x(:, 1) + x(:, 2))
    x(:, 2) = x(:, 1) - x(:, 2)

    y(:, 1) = y(:, 1) + y(:, 2)
    y(:, 2) = y(:, 1) - 2.0d0*y(:, 2)

    do i = 1, 5
        if (abs(x(i, 1) - dble(50 + i)) > 1.0d-12) error stop 1
        if (abs(x(i, 2) + 50.0d0) > 1.0d-12) error stop 2
        if (abs(y(i, 1) - dble(100 + 5*i)) > 1.0d-12) error stop 3
        if (abs(y(i, 2) + dble(100 + i)) > 1.0d-12) error stop 4
    end do
end program
