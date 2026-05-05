module c_backend_class_subroutine_receiver_cleanup_ref_01_m
implicit none

type :: payload_type
    character(:), allocatable :: value
end type

type :: base_type
    type(payload_type), allocatable :: payload
contains
    procedure :: touch
end type

type, extends(base_type) :: child_type
end type

type :: holder_type
    type(child_type), allocatable :: item
end type

contains

subroutine touch(self, text)
    class(base_type), intent(in) :: self
    character(:), allocatable, intent(out) :: text

    text = self%payload%value
end subroutine

subroutine exercise()
    type(holder_type) :: holder
    character(:), allocatable :: text

    allocate(child_type :: holder%item)
    allocate(holder%item%payload)
    holder%item%payload%value = "payload"
    call holder%item%touch(text)
    if (text /= "payload") error stop
    deallocate(text)
    deallocate(holder%item)
end subroutine

end module

program c_backend_class_subroutine_receiver_cleanup_ref_01
use c_backend_class_subroutine_receiver_cleanup_ref_01_m
call exercise()
end program
