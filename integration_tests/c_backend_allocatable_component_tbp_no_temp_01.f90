module c_backend_allocatable_component_tbp_no_temp_01_mod
implicit none

type :: leaf
    integer :: value = 0
contains
    procedure :: read_value
end type

type :: holder
    type(leaf), allocatable :: item
contains
    procedure :: read_item
end type

contains

subroutine read_value(self, value)
class(leaf), intent(in) :: self
integer, intent(out) :: value

value = self%value
end subroutine

subroutine read_item(self, value)
class(holder), intent(in) :: self
integer, intent(out) :: value

if (.not. allocated(self%item)) error stop
call self%item%read_value(value)
end subroutine

end module

program c_backend_allocatable_component_tbp_no_temp_01
use c_backend_allocatable_component_tbp_no_temp_01_mod, only: holder
implicit none

type(holder) :: box
integer :: value

allocate(box%item)
box%item%value = 42

call box%read_item(value)
if (value /= 42) error stop
end program
