program c_backend_iso_fortran_env_no_data_01
use iso_fortran_env, only: output_unit, error_unit, int32, integer_kinds, real_kinds
implicit none

integer :: x

x = output_unit + error_unit + int32 + integer_kinds(3) + real_kinds(2)

if (x /= 22) error stop
if (size(integer_kinds) /= 4) error stop
if (size(real_kinds) /= 2) error stop
end program
