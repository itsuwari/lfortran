module c_backend_module_alloc_comp_resize_01_m
    implicit none

    type item
        logical :: running
    end type

    type holder
        type(item), allocatable :: record(:)
    end type

    type(holder) :: h

contains

    subroutine resize(var)
        type(item), allocatable, intent(inout) :: var(:)
        if (.not. allocated(var)) then
            allocate(var(2))
        end if
    end subroutine

    subroutine run()
        call resize(h%record)
        if (size(h%record) /= 2) error stop 1
        h%record(1)%running = .true.
        if (.not. h%record(1)%running) error stop 2
    end subroutine

end module

program c_backend_module_alloc_comp_resize_01
    use c_backend_module_alloc_comp_resize_01_m, only: run
    call run()
end program
