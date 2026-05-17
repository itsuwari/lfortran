module c_backend_module_reachability_private_unused_01_mod
implicit none
private
public :: public_value

contains

integer function public_value(x)
integer, intent(in) :: x
public_value = used_private(x)
end function

integer function used_private(x)
integer, intent(in) :: x
used_private = x + 17
end function

integer function unused_private(x)
integer, intent(in) :: x
unused_private = x + 8675309
end function

end module

program c_backend_module_reachability_private_unused_01
use c_backend_module_reachability_private_unused_01_mod, only: public_value
implicit none

if (public_value(5) /= 22) error stop

end program
