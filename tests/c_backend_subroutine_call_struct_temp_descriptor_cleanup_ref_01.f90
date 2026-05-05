module c_backend_subroutine_call_struct_temp_descriptor_cleanup_ref_01_m
implicit none

type :: spec_type
    real, allocatable :: kpair(:, :)
end type

type :: holder_type
    real, allocatable :: copied(:, :)
end type

contains

    function make_spec(n) result(spec)
        integer, intent(in) :: n
        type(spec_type) :: spec

        allocate(spec%kpair(n, n), source=1.0)
    end function

    subroutine take_spec(holder, spec)
        type(holder_type), intent(inout) :: holder
        type(spec_type), intent(in) :: spec

        holder%copied = spec%kpair
    end subroutine

    subroutine exercise()
        type(holder_type) :: holder

        call take_spec(holder, make_spec(2))
        if (.not. allocated(holder%copied)) error stop
        if (sum(holder%copied) /= 4.0) error stop
    end subroutine

end module

program c_backend_subroutine_call_struct_temp_descriptor_cleanup_ref_01
use c_backend_subroutine_call_struct_temp_descriptor_cleanup_ref_01_m, only: exercise
implicit none

call exercise()

end program
