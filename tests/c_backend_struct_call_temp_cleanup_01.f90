module c_backend_struct_call_temp_cleanup_01_m
implicit none

type :: base_type
contains
    procedure :: update => base_update
end type

type, extends(base_type) :: child_type
    integer, allocatable :: values(:)
contains
    procedure :: update => child_update
end type

type :: holder_type
    type(child_type), allocatable :: item
contains
    procedure :: update => holder_update
end type

contains

subroutine base_update(self)
    class(base_type), intent(in) :: self
end subroutine

subroutine child_update(self)
    class(child_type), intent(in) :: self
    if (.not. allocated(self%values)) error stop
    if (self%values(1) /= 7) error stop
end subroutine

subroutine holder_update(self)
    class(holder_type), intent(in) :: self
    if (allocated(self%item)) call self%item%update()
end subroutine

subroutine run()
    type(holder_type) :: holder
    allocate(child_type :: holder%item)
    allocate(holder%item%values(1))
    holder%item%values = 7
    call holder%update()
    if (.not. allocated(holder%item%values)) error stop
    if (holder%item%values(1) /= 7) error stop
end subroutine

end module

program c_backend_struct_call_temp_cleanup_01
use c_backend_struct_call_temp_cleanup_01_m, only: run
implicit none
call run()
end program
