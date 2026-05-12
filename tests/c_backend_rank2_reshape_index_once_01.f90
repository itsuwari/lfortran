program c_backend_rank2_reshape_index_once_01
implicit none

real(8) :: a(3, 3)
real(8) :: x1, x2, x3, x4, x5, x6, x7, x8, x9
real(8) :: scale

x1 = 1.0d0
x2 = 2.0d0
x3 = 3.0d0
x4 = 4.0d0
x5 = 5.0d0
x6 = 6.0d0
x7 = 7.0d0
x8 = 8.0d0
x9 = 9.0d0
scale = -0.5d0

a(:, :) = reshape([x1, x2, x3, x4, x5, x6, x7, x8, x9], &
    shape=[3, 3]) * scale

if (abs(a(1, 1) + 0.5d0) > 1.0d-12) error stop
if (abs(a(3, 3) + 4.5d0) > 1.0d-12) error stop

end program c_backend_rank2_reshape_index_once_01
