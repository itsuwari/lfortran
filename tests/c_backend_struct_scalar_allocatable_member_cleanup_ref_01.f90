module c_backend_struct_scalar_allocatable_member_cleanup_ref_01_m
implicit none

type :: run_config
    integer, allocatable :: charge
    integer, allocatable :: spin
    integer, allocatable :: max_iter
end type

contains

    subroutine exercise()
        type(run_config) :: config

        allocate(config%charge)
        allocate(config%spin)
        allocate(config%max_iter)

        config%charge = 0
        config%spin = 2
        config%max_iter = 800

        if (.not. allocated(config%charge)) error stop
        if (.not. allocated(config%spin)) error stop
        if (.not. allocated(config%max_iter)) error stop
    end subroutine

end module

program c_backend_struct_scalar_allocatable_member_cleanup_ref_01
use c_backend_struct_scalar_allocatable_member_cleanup_ref_01_m, only: exercise
implicit none

call exercise()

end program
