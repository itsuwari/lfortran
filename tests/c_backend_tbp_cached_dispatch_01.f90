module c_backend_tbp_cached_dispatch_m
implicit none

type, abstract :: base_t
contains
    procedure(value_iface), deferred :: value
    procedure(apply_iface), deferred :: apply
end type

abstract interface
    integer function value_iface(self, scale)
        import :: base_t
        class(base_t), intent(in) :: self
        integer, intent(in) :: scale
    end function

    subroutine apply_iface(self, value)
        import :: base_t
        class(base_t), intent(in) :: self
        integer, intent(inout) :: value
    end subroutine
end interface

type, extends(base_t) :: child_t
    integer :: x
contains
    procedure :: value => child_value
    procedure :: apply => child_apply
end type

contains

integer function child_value(self, scale)
class(child_t), intent(in) :: self
integer, intent(in) :: scale

child_value = self%x * scale
end function

subroutine child_apply(self, value)
class(child_t), intent(in) :: self
integer, intent(inout) :: value

value = value + self%x
end subroutine

integer function use_dynamic(obj)
class(base_t), intent(in) :: obj
integer :: tmp

tmp = obj%value(5)
call obj%apply(tmp)
use_dynamic = tmp
end function

integer function sum_dynamic_bound(obj)
class(base_t), intent(in) :: obj
integer :: i

sum_dynamic_bound = 0
do i = 1, obj%value(2)
    sum_dynamic_bound = sum_dynamic_bound + i
end do
end function

end module

program main
use c_backend_tbp_cached_dispatch_m
implicit none

type(child_t) :: child

child%x = 7
if (use_dynamic(child) /= 42) error stop
if (sum_dynamic_bound(child) /= 105) error stop
end program
