module c_backend_reachable_type_prune_01_mod
implicit none
private
public :: reachable_type_value

type :: unused_local_type_975318642
    integer :: marker_975318642
end type

contains

integer function reachable_type_value(x)
integer, intent(in) :: x
reachable_type_value = x + 1
end function

end module
