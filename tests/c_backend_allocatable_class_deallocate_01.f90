module c_backend_allocatable_class_deallocate_01_m
implicit none

type record_type
    integer, allocatable :: values(:)
end type

contains

subroutine run()
    class(record_type), allocatable :: item
    allocate(record_type :: item)
    allocate(item%values(3))
    item%values = [1, 2, 3]
    if (item%values(3) /= 3) error stop
    deallocate(item)
end subroutine

end module

program c_backend_allocatable_class_deallocate_01
use c_backend_allocatable_class_deallocate_01_m, only: run
implicit none
call run()
end program
