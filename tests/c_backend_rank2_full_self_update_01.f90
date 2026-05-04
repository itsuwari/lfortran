program c_backend_rank2_full_self_update_01
implicit none

real(8), allocatable :: a(:, :), b(:, :), c(:, :)
real(8) :: d(3, 3), scale
integer :: i, j

allocate(a(3, 3), b(3, 3), c(3, 3))
scale = 3.0_8

do j = 1, 3
    do i = 1, 3
        a(i, j) = real(i + 10*j, 8)
        b(i, j) = real(i + j, 8)
        c(i, j) = real(100 + i + 10*j, 8)
        d(i, j) = real(2*i + j, 8)
    end do
end do

a(:, :) = a + b*2.0_8
a(:, :) = a + d*scale
c(2:3, :2) = c(2:3, :2) + b(2:3, :2)

do j = 1, 3
    do i = 1, 3
        if (abs(a(i, j) - real(i + 10*j + 2*(i + j) + 3*(2*i + j), 8)) > 1.0e-12_8) error stop
        if (i >= 2 .and. j <= 2) then
            if (abs(c(i, j) - real(100 + i + 10*j + i + j, 8)) > 1.0e-12_8) error stop
        else
            if (abs(c(i, j) - real(100 + i + 10*j, 8)) > 1.0e-12_8) error stop
        end if
    end do
end do

end program
