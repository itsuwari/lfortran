module c_backend_imported_member_type_mod_01
implicit none
private
public :: enum_holder_24682468, enum_values_24682468

type :: enum_holder_24682468
    integer :: first
    integer :: second
end type

type(enum_holder_24682468), parameter :: enum_values_24682468 = &
    enum_holder_24682468(11, 22)

end module
