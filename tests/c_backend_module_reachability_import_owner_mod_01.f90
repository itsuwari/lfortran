module c_backend_module_reachability_import_owner_mod_01
implicit none
private
public :: imported_value

contains

integer function imported_value(x)
integer, intent(in) :: x
imported_value = x + 24681357
end function

end module
