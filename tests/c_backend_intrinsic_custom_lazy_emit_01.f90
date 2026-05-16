program c_backend_intrinsic_custom_lazy_emit_01
implicit none

integer :: unit

open(newunit=unit, status="scratch")
write(unit, *) 42
close(unit)

end program c_backend_intrinsic_custom_lazy_emit_01
