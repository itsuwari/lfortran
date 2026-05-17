module c_backend_tbp_exact_class_fast_path_m
implicit none

type, abstract :: base_t
contains
    procedure(value_iface), deferred :: value
end type

abstract interface
    integer function value_iface(self, scale)
        import :: base_t
        class(base_t), intent(in) :: self
        integer, intent(in) :: scale
    end function
end interface

type, extends(base_t) :: child_t
    integer :: x
contains
    procedure :: value => child_value
end type

type, extends(child_t) :: grandchild_t
    integer :: y
contains
    procedure :: value => grandchild_value
end type

contains

integer function child_value(self, scale)
class(child_t), intent(in) :: self
integer, intent(in) :: scale

child_value = self%x * scale
end function

integer function grandchild_value(self, scale)
class(grandchild_t), intent(in) :: self
integer, intent(in) :: scale

grandchild_value = self%x * scale + self%y
end function

integer function class_child_value(obj)
class(child_t), intent(in) :: obj

class_child_value = obj%value(5)
end function

end module

program main
use c_backend_tbp_exact_class_fast_path_m
implicit none

type(child_t) :: child
type(grandchild_t) :: grandchild

child%x = 7
grandchild%x = 7
grandchild%y = 1000

if (class_child_value(child) /= 35) error stop
if (class_child_value(grandchild) /= 1035) error stop
end program
