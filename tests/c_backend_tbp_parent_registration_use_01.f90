program c_backend_tbp_parent_registration_use_01
use c_backend_tbp_parent_registration_mod_01, only: child_t
implicit none

type(child_t) :: child

if (child%value() /= 2) error stop
end program
