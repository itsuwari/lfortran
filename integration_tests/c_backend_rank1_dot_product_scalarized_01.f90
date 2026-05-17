program c_backend_rank1_dot_product_scalarized_01
implicit none

real(8) :: a12(3), a32(3), expected(3)

a12 = [1.0_8, 0.0_8, 0.0_8]
a32 = [2.0_8, 3.0_8, 4.0_8]
expected = [0.0_8, 3.0_8, 4.0_8]

a32 = a32 - a12 * dot_product(a32, a12)

if (any(abs(a32 - expected) > 1.0e-12_8)) error stop

end program
