program use_non_intrinsic_iso_c_binding_01
use, non_intrinsic :: iso_c_binding, only: my_value
implicit none

if (my_value /= 7) error stop

end program
