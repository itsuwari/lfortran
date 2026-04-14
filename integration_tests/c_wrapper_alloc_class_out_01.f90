module c_wrapper_alloc_class_out_01
    type :: base
        integer :: n
    end type
contains

    subroutine make(x)
        class(base), allocatable, intent(out) :: x

        allocate(x)
        x%n = 7
    end subroutine

    subroutine test_out_slot()
        class(base), allocatable :: y

        call make(y)
        if (.not. allocated(y)) error stop 1
        if (y%n /= 7) error stop 2
    end subroutine

end module

program main
    use c_wrapper_alloc_class_out_01

    call test_out_slot()
end program
