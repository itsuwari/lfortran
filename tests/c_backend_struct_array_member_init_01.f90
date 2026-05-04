module c_backend_struct_array_member_init_01_m
implicit none

type record_type
    character(len=:), allocatable :: label
    real, allocatable :: values(:)
    real, allocatable :: matrix(:, :)
end type

contains

subroutine run()
    type(record_type), allocatable :: records(:)
    allocate(records(2))
    allocate(records(1)%values(2))
    records(1)%values = [1.0, 2.0]
    if (abs(records(1)%values(2) - 2.0) > 1e-6) error stop
end subroutine

end module

program c_backend_struct_array_member_init_01
use c_backend_struct_array_member_init_01_m, only: run
implicit none
call run()
end program
