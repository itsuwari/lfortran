module separate_compilation_53a
implicit none

type :: parent_t
contains
    procedure :: value => parent_value
end type

type, extends(parent_t) :: child_t
end type

contains

integer function parent_value(self)
    class(parent_t), intent(in) :: self

    parent_value = 53
end function

integer function dispatch_value(item)
    class(parent_t), intent(in) :: item

    dispatch_value = item%value()
end function

end module
