program c_backend_local_scalar_allocatable_cleanup_ref_01
implicit none

real, allocatable :: charge

allocate(charge)
charge = 1.25

if (.not. allocated(charge)) error stop
if (charge /= 1.25) error stop

end program
