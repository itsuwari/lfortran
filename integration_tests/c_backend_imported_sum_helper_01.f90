program c_backend_imported_sum_helper_01
use c_backend_imported_sum_helper_01_mod, only: imported_sum
implicit none

real(8) :: a(2, 3), b(3), c(3)

a = reshape([1.0_8, 2.0_8, 3.0_8, 4.0_8, 5.0_8, 6.0_8], shape(a))
call imported_sum(a, b)
c = sum(a, 1)

if (any(abs(b - [3.0_8, 7.0_8, 11.0_8]) > 1.0e-12_8)) error stop
if (any(abs(c - b) > 1.0e-12_8)) error stop

end program
