program c_backend_intrinsic_ieee_lazy_emit_01
use, intrinsic :: ieee_arithmetic, only: ieee_is_nan
implicit none

real(8) :: value

value = 1.0_8
if (ieee_is_nan(value)) error stop

end program c_backend_intrinsic_ieee_lazy_emit_01
