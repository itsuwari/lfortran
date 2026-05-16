module c_backend_tbp_force_link_guard_01_m
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

integer function sum_dynamic(obj, n)
class(base_t), intent(in) :: obj
integer, intent(in) :: n
integer :: i

sum_dynamic = 0
do i = 1, n
    sum_dynamic = sum_dynamic + obj%value()
end do
end function

end module

program c_backend_tbp_force_link_guard_01
use c_backend_tbp_force_link_guard_01_m
implicit none

type(child_t) :: child

child%x = 7
if (sum_dynamic(child, 5) /= 35) error stop 1
end program
