module c_backend_proc_pointer_component_01_m
implicit none

abstract interface
    subroutine callback()
    end subroutine
end interface

type :: holder
    procedure(callback), pointer, nopass :: proc => null()
end type

integer :: hits = 0

contains

subroutine set_proc(self, proc)
    type(holder), intent(out) :: self
    procedure(callback) :: proc
    self%proc => proc
end subroutine

subroutine mark()
    hits = hits + 1
end subroutine

end module

program c_backend_proc_pointer_component_01
use c_backend_proc_pointer_component_01_m, only: holder, hits, mark, set_proc
implicit none
type(holder) :: item

call set_proc(item, mark)
call item%proc()
if (hits /= 1) error stop
end program
