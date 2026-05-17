module c_backend_imported_type_prune_01_mod
use c_backend_imported_type_prune_mod_01, only: imported_value
implicit none
private
public :: consumer_value

contains

integer function consumer_value(x)
integer, intent(in) :: x
consumer_value = imported_value(x)
end function

end module
