module c_backend_struct_array_member_cleanup_01_m
implicit none

type :: record_type
    character(:), allocatable :: label
    real, allocatable :: values(:)
end type

type :: holder_type
    type(record_type), allocatable :: records(:)
end type

contains

subroutine fill(holder)
    type(holder_type), intent(inout) :: holder

    allocate(holder%records(2))
    holder%records(1)%label = "mno2"
    allocate(holder%records(1)%values(2))
    holder%records(1)%values = [1.0, 2.0]
    holder%records(2)%label = "h2o"
end subroutine

subroutine run()
    type(holder_type) :: holder

    call fill(holder)
    if (holder%records(1)%label /= "mno2") error stop
    if (holder%records(1)%values(2) /= 2.0) error stop
    if (holder%records(2)%label /= "h2o") error stop
end subroutine

end module

program c_backend_struct_array_member_cleanup_01
use c_backend_struct_array_member_cleanup_01_m, only: run
implicit none

call run()
end program
