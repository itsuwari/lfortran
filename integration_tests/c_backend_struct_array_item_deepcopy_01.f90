module c_backend_struct_array_item_deepcopy_mod
    implicit none

    type :: test_type
        character(len=:), allocatable :: name
        logical :: should_fail = .false.
    end type

contains

    subroutine run_one(test)
        type(test_type), intent(in) :: test

        if (.not. allocated(test%name)) error stop
        if (test%name /= "beta") error stop
        if (test%should_fail) error stop
    end subroutine

    subroutine run_suite()
        type(test_type), allocatable :: testsuite(:)
        integer :: ii

        allocate(testsuite(2))
        testsuite(1)%name = "alpha"
        testsuite(1)%should_fail = .true.
        testsuite(2)%name = "beta"
        testsuite(2)%should_fail = .false.

        ii = 2
        call run_one(testsuite(ii))
    end subroutine

end module

program c_backend_struct_array_item_deepcopy_01
    use c_backend_struct_array_item_deepcopy_mod, only: run_suite
    implicit none

    call run_suite()
end program
