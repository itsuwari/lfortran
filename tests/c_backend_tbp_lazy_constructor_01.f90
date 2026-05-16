module c_backend_tbp_lazy_constructor_m
implicit none
private
public :: check_value

type, abstract :: base_t
contains
    procedure(value_iface), deferred :: value
end type

abstract interface
    integer function value_iface(self)
        import :: base_t
        class(base_t), intent(in) :: self
    end function
end interface

type, extends(base_t) :: child_t
    integer :: x
contains
    procedure :: value => child_value
end type

contains

integer function child_value(self)
class(child_t), intent(in) :: self

child_value = self%x
end function

integer function use_dynamic(obj)
class(base_t), intent(in) :: obj

use_dynamic = obj%value()
end function

integer function check_value()
type(child_t) :: child

child%x = 17
check_value = use_dynamic(child)
end function

end module

program main
use c_backend_tbp_lazy_constructor_m, only: check_value
implicit none

if (check_value() /= 17) error stop
end program
