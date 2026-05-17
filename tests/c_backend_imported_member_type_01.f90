module c_backend_imported_member_type_01
use c_backend_imported_member_type_mod_01, only: enum_values_24682468
implicit none
private
public :: imported_member_value

contains

integer function imported_member_value()
block
imported_member_value = enum_values_24682468%second
end block
end function

end module
