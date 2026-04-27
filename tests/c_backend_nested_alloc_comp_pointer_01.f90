module c_backend_nested_alloc_comp_pointer_01_m
    implicit none

    type item
        integer :: value
    end type

    type inner
        type(item), allocatable :: record(:)
    end type

    type outer
        type(inner) :: dict
    end type

contains

    subroutine resize(var)
        type(item), allocatable, intent(inout) :: var(:)
        if (.not. allocated(var)) then
            allocate(var(2))
        end if
    end subroutine

    subroutine push(self)
        type(inner), intent(inout) :: self
        call resize(self%record)
    end subroutine

    subroutine run()
        type(outer), pointer :: obj
        allocate(obj)
        call push(obj%dict)
        if (size(obj%dict%record) /= 2) error stop 1
        obj%dict%record(1)%value = 42
        if (obj%dict%record(1)%value /= 42) error stop 2
    end subroutine

end module

program c_backend_nested_alloc_comp_pointer_01
    use c_backend_nested_alloc_comp_pointer_01_m, only: run
    call run()
end program
