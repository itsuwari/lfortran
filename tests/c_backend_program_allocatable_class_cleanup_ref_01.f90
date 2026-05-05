module c_backend_program_allocatable_class_cleanup_ref_01_m
implicit none

type :: payload_type
    character(:), allocatable :: value
end type

type :: base_type
    type(payload_type), allocatable :: payload
end type

type, extends(base_type) :: child_type
end type

end module

program c_backend_program_allocatable_class_cleanup_ref_01
use c_backend_program_allocatable_class_cleanup_ref_01_m
implicit none

class(base_type), allocatable :: item

allocate(child_type :: item)
allocate(item%payload)
item%payload%value = "payload"

if (item%payload%value /= "payload") error stop

end program
