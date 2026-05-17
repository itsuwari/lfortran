module c_backend_imported_type_prune_mod_01
implicit none
private
public :: imported_value

type :: unused_imported_type_1357911
    integer :: member_1357911
end type

contains

integer function imported_value(x)
integer, intent(in) :: x
imported_value = x + 1
end function

end module
