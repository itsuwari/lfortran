program c_backend_rank2_reshape_var_scalarized_01
implicit none

real(8) :: src(9), a(3, 3), vec(3), result(3)
real(8) :: scale
integer :: i

do i = 1, 9
    src(i) = dble(i)
end do
scale = -0.5d0
vec = [2.0d0, -1.0d0, 0.5d0]

a(:, :) = reshape(src, shape=[3, 3]) * scale
result = matmul(a, vec)

if (any(result /= result)) error stop
if (abs(result(1) + 0.75d0) > 1.0d-12) error stop
if (abs(result(2) + 1.5d0) > 1.0d-12) error stop
if (abs(result(3) + 2.25d0) > 1.0d-12) error stop

end program c_backend_rank2_reshape_var_scalarized_01
