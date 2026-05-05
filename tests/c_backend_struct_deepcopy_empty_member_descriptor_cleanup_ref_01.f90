module c_backend_struct_deepcopy_empty_member_descriptor_cleanup_ref_01_m
implicit none

type :: record_type
    real, allocatable :: values(:)
end type

type :: dict_type
    type(record_type), allocatable :: record(:)
end type

contains

    subroutine fill(dict)
        type(dict_type), intent(inout) :: dict

        allocate(dict%record(1))
        allocate(dict%record(1)%values(2), source=2.0)
    end subroutine

    subroutine exercise()
        type(dict_type) :: source
        type(dict_type), allocatable :: dest

        call fill(source)
        allocate(dest)
        dest = source
        if (.not. allocated(dest%record)) error stop
        if (sum(dest%record(1)%values) /= 4.0) error stop
    end subroutine

end module

program c_backend_struct_deepcopy_empty_member_descriptor_cleanup_ref_01
use c_backend_struct_deepcopy_empty_member_descriptor_cleanup_ref_01_m, only: exercise
implicit none

call exercise()

end program
