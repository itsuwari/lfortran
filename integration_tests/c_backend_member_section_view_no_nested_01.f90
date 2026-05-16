program c_backend_member_section_view_no_nested_01
implicit none

type potential_t
    real(8), allocatable :: v(:, :, :)
end type

type(potential_t) :: pot
real(8), allocatable :: a(:, :, :), y(:), expected(:)
integer :: i, j, k, iat

allocate(pot%v(3, 2, 1), a(3, 3, 2), y(3), expected(3))

do iat = 1, 2
    do i = 1, 3
        pot%v(i, iat, 1) = real(10*iat + i, 8) * 0.125_8
    end do
end do

do k = 1, 2
    do j = 1, 3
        do i = 1, 3
            a(i, j, k) = real(i + 2*j + 3*k, 8) * 0.25_8
        end do
    end do
end do

y(:) = 0.0_8
expected(:) = 0.0_8

do k = 1, 2
    iat = k
    y(:) = y(:) + matmul(a(:, :, k), pot%v(:, iat, 1))
    do i = 1, 3
        do j = 1, 3
            expected(i) = expected(i) + a(i, j, k) * pot%v(j, iat, 1)
        end do
    end do
end do

do i = 1, 3
    if (abs(y(i) - expected(i)) > 1.0e-12_8) error stop
end do

end program
