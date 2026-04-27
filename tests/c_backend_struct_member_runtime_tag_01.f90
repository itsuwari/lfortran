module member_tag_m
implicit none

type, abstract :: base_t
contains
    procedure(value_iface), deferred :: value
end type

abstract interface
    subroutine value_iface(self, out)
        import :: base_t
        class(base_t), intent(in) :: self
        integer, intent(out) :: out
    end subroutine
end interface

type, extends(base_t) :: child_t
contains
    procedure :: value => child_value
end type

type :: holder_t
    type(child_t) :: child
end type

contains

subroutine reset_holder(holder)
    type(holder_t), intent(out) :: holder
end subroutine

subroutine dispatch_base(obj, out)
    class(base_t), intent(in) :: obj
    integer, intent(out) :: out
    call obj%value(out)
end subroutine

subroutine child_value(self, out)
    class(child_t), intent(in) :: self
    integer, intent(out) :: out
    out = 42
end subroutine

end module

program main
use member_tag_m
implicit none
type(holder_t) :: holder
integer :: out

call reset_holder(holder)
call dispatch_base(holder%child, out)
print *, out
end program
