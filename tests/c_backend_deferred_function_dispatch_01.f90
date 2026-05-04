module deferred_function_dispatch_m
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

contains

integer function child_value(self, scale)
class(child_t), intent(in) :: self
integer, intent(in) :: scale

child_value = self%x * scale
end function

integer function use_value(obj)
class(base_t), intent(in) :: obj

use_value = 3 + obj%value(5)
end function

end module

program main
use deferred_function_dispatch_m
implicit none

type(child_t) :: child
integer :: got

child%x = 7
got = use_value(child)
print *, got
end program
