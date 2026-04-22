module allocatable_aggregate_dummy_slot_01_m
    implicit none

    type :: error_t
        character(:), allocatable :: message
    end type error_t

contains

    subroutine set_error(error)
        type(error_t), allocatable :: error

        allocate(error)
        error%message = "nested allocatable dummy propagated"
    end subroutine set_error

    subroutine run_case()
        type(error_t), allocatable :: error

        call set_error(error)
        if (.not.allocated(error)) error stop "error not allocated"
        if (.not.allocated(error%message)) error stop "message not allocated"
        print *, trim(error%message)
    end subroutine run_case

end module allocatable_aggregate_dummy_slot_01_m

program allocatable_aggregate_dummy_slot_01
    use allocatable_aggregate_dummy_slot_01_m, only: run_case

    implicit none

    call run_case()
end program allocatable_aggregate_dummy_slot_01
