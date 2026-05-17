program c_backend_inline_sum_expr_01
implicit none

real(8) :: a(2, 3), b(2, 3), c(2, 3, 2), s
integer :: i, j

do j = 1, 3
    do i = 1, 2
        a(i, j) = real(i + j, 8)
    end do
end do

b = 2.0d0 * a
c(:, :, 1) = a
c(:, :, 2) = -a
s = sum(a * b)

if (abs(s - 158.0d0) > 1.0d-12) error stop

s = sum(c(:, :, 1) * b)

if (abs(s - 158.0d0) > 1.0d-12) error stop

end program c_backend_inline_sum_expr_01
