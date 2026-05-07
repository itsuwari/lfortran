program c_backend_rank2_scalarized_precedence_01
implicit none

real(8) :: a(2, 2), identity(2, 2), vec(2)
real(8) :: x, y, scale

a = 0.0d0
identity = reshape([1.0d0, 0.0d0, 0.0d0, 1.0d0], [2, 2])
vec = [2.0d0, 3.0d0]
x = 5.0d0
y = 2.0d0
scale = 3.0d0

a(:, :) = a + scale * ((x + y) * identity &
    - spread(vec, 1, 2) * spread(vec, 2, 2))

if (abs(a(1, 1) - 9.0d0) > 1.0d-12) error stop
if (abs(a(2, 1) + 18.0d0) > 1.0d-12) error stop
if (abs(a(1, 2) + 18.0d0) > 1.0d-12) error stop
if (abs(a(2, 2) + 6.0d0) > 1.0d-12) error stop
if (abs(sum(a) + 33.0d0) > 1.0d-12) error stop
end program
