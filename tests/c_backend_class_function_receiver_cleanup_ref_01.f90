module c_backend_class_function_receiver_cleanup_ref_01_m
implicit none

type :: base_type
    character(:), allocatable :: label
contains
    procedure :: info
end type

type, extends(base_type) :: child_type
end type

type :: holder_type
    type(child_type), allocatable :: item
end type

contains

function info(self) result(str)
    class(base_type), intent(in) :: self
    character(:), allocatable :: str
    str = self%label
end function

subroutine exercise()
    type(holder_type) :: holder
    character(:), allocatable :: text

    allocate(child_type :: holder%item)
    holder%item%label = "payload"
    text = holder%item%info()
    if (text /= "payload") error stop
    deallocate(text)
    deallocate(holder%item)
end subroutine

end module

program c_backend_class_function_receiver_cleanup_ref_01
use c_backend_class_function_receiver_cleanup_ref_01_m
call exercise()
end program
