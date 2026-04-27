module class_nondeferred_dispatch
implicit none

type, abstract :: base_t
contains
    procedure :: add
end type

type, extends(base_t) :: child_t
contains
    procedure :: add => child_add
end type

contains

subroutine add(self, value)
class(base_t), intent(in) :: self
integer, intent(inout) :: value
end subroutine

subroutine child_add(self, value)
class(child_t), intent(in) :: self
integer, intent(inout) :: value

value = value + 7
end subroutine

subroutine apply_add(obj, value)
class(base_t), intent(in) :: obj
integer, intent(inout) :: value

call obj%add(value)
end subroutine

end module

program main
use class_nondeferred_dispatch
implicit none

type(child_t) :: child
integer :: value

value = 1
call apply_add(child, value)
print *, value
end program
