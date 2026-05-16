module c_backend_tbp_lazy_registration_m
implicit none

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

end module

program main
use c_backend_tbp_lazy_registration_m
implicit none

type(child_t) :: child

child%x = 42
if (use_dynamic(child) /= 42) error stop
end program
