program c_backend_rank2_reshape_constructor_scalarized_01
implicit none

real(8) :: a(3, 3), inertia(3, 3), result(3), vec(3)
real(8) :: scale1, scale2
integer :: i, j

do j = 1, 3
    do i = 1, 3
        inertia(i, j) = dble(i + 3*(j - 1))
    end do
end do

scale1 = 0.25d0
scale2 = -2.0d0
vec = [2.0d0, -1.0d0, 0.5d0]

a(:, :) = reshape([&
    inertia(1, 1)**2 / inertia(1, 1), &
    inertia(2, 1)**2 / inertia(2, 1), &
    inertia(3, 1)**2 / inertia(3, 1), &
    inertia(1, 2)**2 / inertia(1, 2), &
    inertia(2, 2)**2 / inertia(2, 2), &
    inertia(3, 2)**2 / inertia(3, 2), &
    inertia(1, 3)**2 / inertia(1, 3), &
    inertia(2, 3)**2 / inertia(2, 3), &
    inertia(3, 3)**2 / inertia(3, 3)], shape=[3, 3]) * scale1 * scale2
result = matmul(a, vec)

if (any(result /= result)) error stop
if (abs(result(1) + 0.75d0) > 1.0d-12) error stop
if (abs(result(2) + 1.5d0) > 1.0d-12) error stop
if (abs(result(3) + 2.25d0) > 1.0d-12) error stop

end program c_backend_rank2_reshape_constructor_scalarized_01
