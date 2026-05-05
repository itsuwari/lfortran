module c_backend_allocatable_char_component_deallocate_cleanup_ref_01_m
implicit none

type :: holder_type
    character(:), allocatable :: value
contains
    procedure :: clear
end type

contains

subroutine clear(self)
    class(holder_type), intent(inout) :: self
    if (allocated(self%value)) deallocate(self%value)
end subroutine

subroutine exercise()
    type(holder_type) :: holder
    holder%value = "owned"
    call holder%clear()
end subroutine

end module

program c_backend_allocatable_char_component_deallocate_cleanup_ref_01
use c_backend_allocatable_char_component_deallocate_cleanup_ref_01_m
call exercise()
end program
