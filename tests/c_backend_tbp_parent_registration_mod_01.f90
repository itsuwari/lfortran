module c_backend_tbp_parent_registration_mod_01
implicit none

type :: parent_t
contains
    procedure :: value => parent_value
end type

type, extends(parent_t) :: child_t
contains
    procedure :: value => child_value
end type

contains

integer function parent_value(self)
class(parent_t), intent(in) :: self

parent_value = 1
end function

integer function child_value(self)
class(child_t), intent(in) :: self

child_value = 2
end function

end module
