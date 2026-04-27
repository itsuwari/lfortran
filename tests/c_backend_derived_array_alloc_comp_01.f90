module c_backend_derived_array_alloc_comp_01_m
    implicit none

    type record_type
        real, allocatable :: array3(:, :, :)
    end type

contains

    subroutine run()
        type(record_type), allocatable :: records(:)
        allocate(records(1))
        allocate(records(1)%array3(2, 2, 2))
        records(1)%array3 = 3.0
        if (abs(records(1)%array3(2, 2, 2) - 3.0) > 1e-6) error stop 1
    end subroutine

end module

program c_backend_derived_array_alloc_comp_01
    use c_backend_derived_array_alloc_comp_01_m, only: run
    call run()
end program
