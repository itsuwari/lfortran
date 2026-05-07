program c_backend_rank2_reshape_scalarized_01
implicit none

real(8) :: aderiv(3, 3), vec(3), result(3)
real(8) :: x1, x2, x3, x4, x5, x6, x7, x8, x9
real(8) :: scale1, scale2

x1 = 1.0d0
x2 = 2.0d0
x3 = 3.0d0
x4 = 4.0d0
x5 = 5.0d0
x6 = 6.0d0
x7 = 7.0d0
x8 = 8.0d0
x9 = 9.0d0
scale1 = 2.0d0
scale2 = -0.25d0
vec = [2.0d0, -1.0d0, 0.5d0]

aderiv(:, :) = reshape([x1, x2, x3, x4, x5, x6, x7, x8, x9], &
    shape=[3, 3]) * scale1 * scale2
result = matmul(aderiv, vec)

if (any(result /= result)) error stop
if (abs(result(1) + 0.75d0) > 1.0d-12) error stop
if (abs(result(2) + 1.50d0) > 1.0d-12) error stop
if (abs(result(3) + 2.25d0) > 1.0d-12) error stop

end program
