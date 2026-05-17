program c_backend_rank1_matmul_self_update_terms_01
implicit none

real(8) :: dg(3), vec(3), a(3, 3), b(3, 3), c(3), d(3)
real(8) :: gradient(3, 2), expected(3)
integer :: iat

dg = [100.0d0, 200.0d0, 300.0d0]
vec = [3.0d0, 5.0d0, 7.0d0]
a = reshape([ &
    1.0d0,  2.0d0,  3.0d0, &
    4.0d0,  5.0d0,  6.0d0, &
    7.0d0,  8.0d0,  9.0d0], shape(a))
b = reshape([ &
    2.0d0, -1.0d0,  4.0d0, &
    0.5d0, 3.0d0, -2.0d0, &
    1.0d0,  6.0d0,  8.0d0], shape(b))
c = [11.0d0, -13.0d0, 17.0d0]
d = [19.0d0, 23.0d0, -29.0d0]

expected = dg + 2.0d0 * vec - matmul(a, c) - matmul(b, d)
dg(:) = dg + 2.0d0 * vec - matmul(a, c) - matmul(b, d)

if (any(abs(dg - expected) > 1.0d-12)) error stop

iat = 2
gradient(:, 1) = [31.0d0, 37.0d0, 41.0d0]
gradient(:, 2) = [43.0d0, 47.0d0, 53.0d0]
expected = gradient(:, iat) + 3.0d0 * matmul(a, c)
gradient(:, iat) = gradient(:, iat) + 3.0d0 * matmul(a, c)

if (any(abs(gradient(:, iat) - expected) > 1.0d-12)) error stop

end program
