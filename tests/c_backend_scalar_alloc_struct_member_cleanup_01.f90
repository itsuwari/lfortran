module c_backend_scalar_alloc_struct_member_cleanup_01_m
implicit none

type :: child_type
    character(:), allocatable :: label
    real, allocatable :: values(:)
end type

type :: concrete_holder
    type(child_type), allocatable :: item
end type

contains

subroutine fill_concrete(holder)
    type(concrete_holder), intent(inout) :: holder

    allocate(holder%item)
    holder%item%label = "concrete"
    allocate(holder%item%values(2))
    holder%item%values = [3.0, 4.0]
end subroutine

subroutine run()
    type(concrete_holder), allocatable :: concrete
    type(concrete_holder) :: local

    allocate(concrete)
    call fill_concrete(concrete)
    if (concrete%item%label /= "concrete") error stop
    if (concrete%item%values(2) /= 4.0) error stop

    call fill_concrete(local)
    if (local%item%label /= "concrete") error stop
    if (local%item%values(2) /= 4.0) error stop
end subroutine

end module

program c_backend_scalar_alloc_struct_member_cleanup_01
use c_backend_scalar_alloc_struct_member_cleanup_01_m, only: run
implicit none

call run()
end program
