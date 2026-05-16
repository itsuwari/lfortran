program c_backend_intrinsic_iso_env_lazy_emit_01
use iso_fortran_env, only: int32
implicit none

integer(int32) :: value

value = 42_int32
if (value /= 42_int32) error stop

end program c_backend_intrinsic_iso_env_lazy_emit_01
