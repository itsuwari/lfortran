module c_backend_module_reachability_nested_01_mod
implicit none
private
public :: public_value

contains

integer function public_value(x)
integer, intent(in) :: x

public_value = twice(x)

contains

integer function twice(y)
integer, intent(in) :: y
twice = 2*y
end function

end function

end module

program c_backend_module_reachability_nested_01
use c_backend_module_reachability_nested_01_mod, only: public_value
implicit none

if (public_value(11) /= 22) error stop

end program
