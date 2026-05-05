module c_backend_local_struct_scalar_member_array_descriptor_cleanup_ref_01_m
implicit none

type :: record_type
    real, allocatable :: values(:)
end type

type :: dict_type
    integer :: n = 0
    type(record_type), allocatable :: record(:)
end type

type :: results_type
    type(dict_type), allocatable :: dict
end type

contains

    subroutine fill(results)
        type(results_type), intent(inout) :: results

        allocate(results%dict)
        results%dict%n = 1
        allocate(results%dict%record(1))
        allocate(results%dict%record(1)%values(2), source=2.0)
    end subroutine

    subroutine exercise()
        type(results_type) :: results

        call fill(results)
        if (.not. allocated(results%dict)) error stop
        if (.not. allocated(results%dict%record)) error stop
        if (sum(results%dict%record(1)%values) /= 4.0) error stop
    end subroutine

end module

program c_backend_local_struct_scalar_member_array_descriptor_cleanup_ref_01
use c_backend_local_struct_scalar_member_array_descriptor_cleanup_ref_01_m, only: exercise
implicit none

call exercise()

end program
